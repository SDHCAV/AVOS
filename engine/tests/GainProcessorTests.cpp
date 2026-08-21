#include "../src/GainProcessor.h"
#include <juce_core/juce_core.h>

class GainProcessorTests : public juce::UnitTest
{
public:
    GainProcessorTests() : juce::UnitTest("GainProcessor", "AudioProcessors") {}

    void runTest() override
    {
        beginTest("Unity gain (1.0) preserves signal");
        {
            GainProcessor gain;
            gain.setGain(1.0f);
            juce::AudioBuffer<float> buffer(1, 4);
            for (int i = 0; i < 4; ++i) buffer.setSample(0, i, 0.8f);

            juce::MidiBuffer midi;
            gain.processBlock(buffer, midi);

            for (int i = 0; i < 4; ++i)
                expectWithinAbsoluteError(buffer.getSample(0, i), 0.8f, 0.0001f);

            logMessage("  [PASS] Unity gain preserves signal amplitude");
        }

        beginTest("Zero gain (0.0) produces silence");
        {
            GainProcessor gain;
            gain.setGain(0.0f);
            juce::AudioBuffer<float> buffer(1, 4);
            for (int i = 0; i < 4; ++i) buffer.setSample(0, i, 0.8f);

            juce::MidiBuffer midi;
            gain.processBlock(buffer, midi);

            for (int i = 0; i < 4; ++i)
                expectWithinAbsoluteError(buffer.getSample(0, i), 0.0f, 0.0001f);

            logMessage("  [PASS] Zero gain completely silences signal");
        }
    }
};

static GainProcessorTests gainProcessorTests;