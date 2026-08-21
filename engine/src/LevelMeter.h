#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <algorithm>

class LevelMeter {
public:
    LevelMeter() : currentLevel(0.0f) {}

    void processBuffer(const juce::AudioBuffer<float>& buffer) {
        float sum = 0.0f;
        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();

        if (numChannels == 0 || numSamples == 0) return;

        for (int channel = 0; channel < numChannels; ++channel) {
            const float* channelData = buffer.getReadPointer(channel);
            for (int i = 0; i < numSamples; ++i) {
                sum += channelData[i] * channelData[i];
            }
        }

        // Calculate Root Mean Square (RMS)
        float rms = std::sqrt(sum / (numChannels * numSamples));
        currentLevel.store(rms, std::memory_order_relaxed);
    }

    float getLevel() const {
        return currentLevel.load(std::memory_order_relaxed);
    }

private:
    std::atomic<float> currentLevel;
};