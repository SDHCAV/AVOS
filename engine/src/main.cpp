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

    // CHANGED: deviceManager is now declared BEFORE the wsServer setup below,
    // so it can be captured by setDeviceListProvider before wsServer.start() runs.
    juce::AudioDeviceManager deviceManager;
    deviceManager.initialiseWithDefaultDevices(2, 2);

    EngineWebSocketServer wsServer(routingGraph);

    // CHANGED: moved this ABOVE wsServer.start() — previously it was set after start(),
    // which left a small window where a very-fast-connecting client could get no device list.
    wsServer.setDeviceListProvider([&deviceManager]() -> juce::StringArray
    {
        juce::StringArray devices;
        if (auto* type = deviceManager.getCurrentDeviceTypeObject())
        {
            devices.addArray(type->getDeviceNames(true));   // inputs
            devices.addArray(type->getDeviceNames(false));  // outputs
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
    routingGraph.connect(routingGraph.getAudioInputNodeID(), 0, gainID, 0);
    routingGraph.connect(gainID, 0, pannerID, 0);
    routingGraph.connect(pannerID, 0, routingGraph.getAudioOutputNodeID(), 0);

    //mixminus
    auto mixMinusPtr = std::make_unique<MixMinusBus>(2); //2 sources, mic (channel 0), zoom (channel 1)
    MixMinusBus* mixMinus = mixMinusPtr.get();
    auto mixMinusID = routingGraph.addNode(std::move(mixMinusPtr));
    mixMinus->setExcludedChannel(1); //exclude zoom return

    routingGraph.connect(routingGraph.getAudioInputNodeID(), 0, mixMinusID, 0); //0 = mic
    routingGraph.connect(routingGraph.getAudioInputNodeID(), 1, mixMinusID, 1); //1 = zoom
    routingGraph.connect(mixMinusID, 0, routingGraph.getAudioOutputNodeID(), 1); //bus output -> zoom send channel 1

    EngineAudioCallback callback(routingGraph);
    deviceManager.addAudioCallback(&callback);

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
    wsServer.stop();   // CHANGED (from way earlier): this now correctly runs before return, not after
    std::cout << "\nEngine shut down cleanly." << std::endl;
    return 0;
}