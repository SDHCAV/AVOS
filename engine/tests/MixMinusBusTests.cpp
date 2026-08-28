#include "../src/MixMinusBus.h"
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

class MixMinusBusTests : public juce::UnitTest
{
public:
    MixMinusBusTests() : juce::UnitTest("MixMinusBus") {}

    void runTest() override
    {
        beginTest("Sums all channels when nothing is excluded");
        {
            MixMinusBus bus(3);
            bus.setExcludedChannel(-1);

            juce::AudioBuffer<float> buffer(3, 4);
            buffer.clear();
            for (int ch = 0; ch < 3; ++ch)
                for (int i = 0; i < 4; ++i)
                    buffer.setSample(ch, i, 1.0f); // each input = constant 1.0

            juce::MidiBuffer midi;
            bus.processBlock(buffer, midi);

            for (int i = 0; i < 4; ++i)
                expectWithinAbsoluteError(buffer.getSample(0, i), 3.0f, 0.0001f); // 1+1+1
        }

        beginTest("Skips the excluded channel (Zoom Return scenario)");
        {
            MixMinusBus bus(2); // 0 = Mic, 1 = Zoom Return
            bus.setExcludedChannel(1);

            juce::AudioBuffer<float> buffer(2, 4);
            buffer.clear();
            for (int i = 0; i < 4; ++i)
            {
                buffer.setSample(0, i, 1.0f); // Mic = 1.0
                buffer.setSample(1, i, 1.0f); // Zoom Return = 1.0 (should be excluded)
            }

            juce::MidiBuffer midi;
            bus.processBlock(buffer, midi);

            // Only Mic should be present - Zoom Return excluded means output = 1.0, not 2.0
            for (int i = 0; i < 4; ++i)
                expectWithinAbsoluteError(buffer.getSample(0, i), 1.0f, 0.0001f);
        }
    }
};

static MixMinusBusTests mixMinusBusTests;