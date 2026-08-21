#include "../src/PannerProcessor.h"
#include <juce_core/juce_core.h>

class PannerProcessorTests : public juce::UnitTest
{
public:
    PannerProcessorTests() : juce::UnitTest("PannerProcessor") {}

    void runTest() override
    {
        beginTest("Center pan (0.0) splits signal equally");
        {
            PannerProcessor panner;
            juce::AudioBuffer<float> buffer(2, 4); // 2 channels (mono in ch 0, stereo out), 4 samples
            buffer.clear();
            for (int i = 0; i < 4; ++i)
                buffer.setSample(0, i, 1.0f); // mono input signal = constant 1.0

            juce::MidiBuffer midi;
            panner.setPanner(0.0f);
            panner.processBlock(buffer, midi);

            for (int i = 0; i < 4; ++i)
            {
                expectWithinAbsoluteError(buffer.getSample(0, i), 0.5f, 0.0001f); // left
                expectWithinAbsoluteError(buffer.getSample(1, i), 0.5f, 0.0001f); // right
            }
        }

        beginTest("Full left pan (-1.0) puts all signal on left, none on right");
        {
            PannerProcessor panner;
            juce::AudioBuffer<float> buffer(2, 4);
            buffer.clear();
            for (int i = 0; i < 4; ++i)
                buffer.setSample(0, i, 1.0f);

            juce::MidiBuffer midi;
            panner.setPanner(-1.0f);
            panner.processBlock(buffer, midi);

            for (int i = 0; i < 4; ++i)
            {
                expectWithinAbsoluteError(buffer.getSample(0, i), 1.0f, 0.0001f); // left = full
                expectWithinAbsoluteError(buffer.getSample(1, i), 0.0f, 0.0001f); // right = silent
            }
        }

        beginTest("Full right pan (1.0) puts all signal on right, none on left");
        {
            PannerProcessor panner;
            juce::AudioBuffer<float> buffer(2, 4);
            buffer.clear();
            for (int i = 0; i < 4; ++i)
                buffer.setSample(0, i, 1.0f);

            juce::MidiBuffer midi;
            panner.setPanner(1.0f);
            panner.processBlock(buffer, midi);

            for (int i = 0; i < 4; ++i)
            {
                expectWithinAbsoluteError(buffer.getSample(0, i), 0.0f, 0.0001f); // left = silent
                expectWithinAbsoluteError(buffer.getSample(1, i), 1.0f, 0.0001f); // right = full
            }
        }

        beginTest("setPanner clamps out-of-range values");
        {
            PannerProcessor panner;
            panner.setPanner(5.0f); // way out of range
            juce::AudioBuffer<float> buffer(2, 1);
            buffer.setSample(0, 0, 1.0f);
            juce::MidiBuffer midi;
            panner.processBlock(buffer, midi);

            // clamped to 1.0 -> full right, left should be 0
            expectWithinAbsoluteError(buffer.getSample(0, 0), 0.0f, 0.0001f);
            expectWithinAbsoluteError(buffer.getSample(1, 0), 1.0f, 0.0001f);
        }
    }
};

static PannerProcessorTests pannerProcessorTests;