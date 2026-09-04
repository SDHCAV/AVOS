#include "RoutingGraph.h"
#include "GainProcessor.h"
#include "PannerProcessor.h"
#include "MixMinusBus.h"

#include "EngineWebSocketServer.h"
#include "EngineAudioCallback.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>

std::string renderMeterBar(float level, int width = 15) {
    int filled = static_cast<int>(level * width * 6.0f);
    filled = std::min(width, std::max(0, filled));

    std::string bar = "[";
    for (int i = 0; i < width; ++i) {
        if (i < filled) bar += "=";
        else bar += " ";
    }
    bar += "]";
    return bar;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;

    RoutingGraph routingGraph;

    juce::AudioDeviceManager deviceManager;

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);

    setup.outputDeviceName = "CABLE Input (VB-Audio Virtual Cable)";
    setup.outputChannels.clear();
    setup.outputChannels.setRange(0, 2, true);
    setup.useDefaultOutputChannels = false;

    juce::String result = deviceManager.initialise(2, 2, nullptr, true, {}, &setup);

    if (result.isNotEmpty())
    {
        std::cerr << "Failed to initialise audio device with virtual cable: " << result << std::endl;
        deviceManager.initialiseWithDefaultDevices(2, 2);
    }

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        std::cout << "Active output device: " << device->getName() << std::endl;
        std::cout << "Active output channels: " << device->getOutputChannelNames().joinIntoString(", ") << std::endl;
    }

    // CHANGED: EngineAudioCallback + addAudioCallback moved up here — BEFORE any
    // graph nodes are created or connected. addAudioCallback triggers
    // audioDeviceAboutToStart(), which calls routingGraph.prepare(), which is what
    // actually gives the graph's input/output I/O nodes a real channel count.
    // Doing this first means every connect() call below happens against
    // properly-configured I/O nodes, instead of nodes that still think they have 0 channels.
    EngineAudioCallback callback(routingGraph);
    deviceManager.addAudioCallback(&callback);

    EngineWebSocketServer wsServer(routingGraph);

    wsServer.setDeviceListProvider([&deviceManager]() -> juce::StringArray
    {
        juce::StringArray devices;
        if (auto* type = deviceManager.getCurrentDeviceTypeObject())
        {
            devices.addArray(type->getDeviceNames(true));
            devices.addArray(type->getDeviceNames(false));
        }
        return devices;
    });

    wsServer.start();

    //gain
    auto gainPtr = std::make_unique<GainProcessor>();
    GainProcessor* gain = gainPtr.get();
    auto gainID = routingGraph.addNode(std::move(gainPtr));

    //panner
    auto pannerPtr = std::make_unique<PannerProcessor>();
    PannerProcessor* panner = pannerPtr.get();
    auto pannerID = routingGraph.addNode(std::move(pannerPtr));

    //connect mic (input)-> gain -> panner -> speakers (output)
    // CHANGED: these should now succeed, since the I/O nodes are already prepared above
    bool ok1 = routingGraph.connect(routingGraph.getAudioInputNodeID(), 0, gainID, 0);
    bool ok2 = routingGraph.connect(gainID, 0, pannerID, 0);
    bool ok3 = routingGraph.connect(pannerID, 0, routingGraph.getAudioOutputNodeID(), 0);

    std::cout << "mic->gain: " << ok1
            << ", gain->panner: " << ok2
            << ", panner->output: " << ok3 << std::endl;

    wsServer.registerEndpoint("mic-1", routingGraph.getAudioInputNodeID(), 0);
    wsServer.registerEndpoint("gain-1", gainID, 0);
    wsServer.registerEndpoint("panner-1", pannerID, 0);
    wsServer.registerEndpoint("speaker-out", routingGraph.getAudioOutputNodeID(), 0);

    //mixminus
    auto mixMinusPtr = std::make_unique<MixMinusBus>(2);
    MixMinusBus* mixMinus = mixMinusPtr.get();
    auto mixMinusID = routingGraph.addNode(std::move(mixMinusPtr));
    mixMinus->setExcludedChannel(1);

    // CHANGED: capturing these too, so we can confirm the mixMinus path also succeeds now
    bool ok4 = routingGraph.connect(routingGraph.getAudioInputNodeID(), 0, mixMinusID, 0);
    bool ok5 = routingGraph.connect(routingGraph.getAudioInputNodeID(), 1, mixMinusID, 1);
    bool ok6 = routingGraph.connect(mixMinusID, 0, routingGraph.getAudioOutputNodeID(), 1);

    std::cout << "mic->mixMinus[0]: " << ok4
            << ", mic->mixMinus[1]: " << ok5
            << ", mixMinus->output[1]: " << ok6 << std::endl;

    wsServer.registerEndpoint("zoomSend-1", routingGraph.getAudioOutputNodeID(), 1);
    wsServer.registerEndpoint("mixminus-1", mixMinusID, 0);

    std::atomic<bool> levelsRunning{true};
    std::thread levelsThread([&]()
    {
        while (levelsRunning)
        {
            float micLevel = callback.inputMeter.getLevel();
            float outLevel = callback.outputMeter.getLevel();

            wsServer.broadcastLevels("mic-1", micLevel, micLevel);
            wsServer.broadcastLevels("speaker-out", outLevel, outLevel);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    std::cout << "========================================" << std::endl;
    std::cout << "  AVOS Audio Engine: Pan & Gain Demo   " << std::endl;
    std::cout << "========================================" << std::endl;

    auto runInteractiveStage = [&](const std::string& label) {
        std::cout << "\n>>> " << label << " <<<" << std::endl;
        std::cout << "Press [ENTER] to continue...\n" << std::endl;

        std::atomic<bool> keepPrinting{true};
        std::thread meterThread([&]() {
            while (keepPrinting) {
                float inLvl = callback.inputMeter.getLevel();
                float outLvl = callback.outputMeter.getLevel();

                std::cout << "\rMic Input: " << renderMeterBar(inLvl)
                          << " | Out: " << renderMeterBar(outLvl) << std::flush;

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });

        std::cin.get();
        keepPrinting = false;
        if (meterThread.joinable()) meterThread.join();
    };

    // Stage 1: Center Pan
    panner->setPanner(0.0f);
    runInteractiveStage("Stage 1: Center Pan (Equal Stereo)");

    // Stage 2: Hard Left
    panner->setPanner(-1.0f);
    runInteractiveStage("Stage 2: Hard Left Pan (-1.0f)");

    // Stage 3: Hard Right
    panner->setPanner(1.0f);
    runInteractiveStage("Stage 3: Hard Right Pan (+1.0f)");

    // Stage 4: Mute Output
    gain->setGain(0.0f);
    runInteractiveStage("Stage 4: Muted (Gain set to 0.0f)");

    deviceManager.removeAudioCallback(&callback);
    levelsRunning = false;
    if (levelsThread.joinable())
        levelsThread.join();
    wsServer.stop();
    std::cout << "\nEngine shut down cleanly." << std::endl;
    return 0;
}