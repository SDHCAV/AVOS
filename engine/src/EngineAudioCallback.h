#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include "RoutingGraph.h"
#include "LevelMeter.h"

class EngineAudioCallback : public juce::AudioIODeviceCallback{
public:
    explicit EngineAudioCallback(RoutingGraph& graphToUse) : routingGraph(graphToUse) {}

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override{
        routingGraph.prepare(device->getCurrentSampleRate(),
            device->getCurrentBufferSizeSamples(),
            device->getActiveInputChannels().countNumberOfSetBits(),
            device->getActiveOutputChannels().countNumberOfSetBits());
    }

    void audioDeviceStopped() override {}

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext&) override{

        juce::AudioBuffer<float> inputBuffer(const_cast<float**>(inputChannelData), numInputChannels, numSamples);
        inputMeter.processBuffer(inputBuffer);

        juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);
        for (int ch = 0; ch < juce::jmin(numInputChannels, numOutputChannels); ++ch)
            buffer.copyFrom(ch, 0, inputChannelData[ch], numSamples);
    
        juce::MidiBuffer midi; 
        routingGraph.processBlock(buffer, midi);

        outputMeter.processBuffer(buffer);
    }
    LevelMeter inputMeter;
    LevelMeter outputMeter;

private:
    RoutingGraph& routingGraph;
};