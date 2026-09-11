#include "../src/GateProcessor.h"
#include <juce_core/juce_core.h>

class GateProcessorTests : public juce::UnitTest
{
public:
    GateProcessorTests() : juce::UnitTest("GateProcessor", "AudioProcessors") {}

    void runTest() override
    {
        beginTest("Signal above threshold passes through mostly unaffected");
        {
            GateProcessor gate(1);
            gate.setThresholdDb(-20.0f);
            gate.setAttackMs(1.0f);    // fast attack so the gate opens almost immediately
            gate.setReleaseMs(50.0f);
            gate.prepareToPlay(44100.0, 512);

            // Loud signal (near 0dB), well above -20dB threshold
            const int numSamples = 2000; // enough samples for attack smoothing to fully open
            juce::AudioBuffer<float> buffer(1, numSamples);
            for (int i = 0; i < numSamples; ++i)
                buffer.setSample(0, i, 0.9f);

            juce::MidiBuffer midi;
            gate.processBlock(buffer, midi);

            // Check the tail end of the block, after attack has had time to fully open
            expectGreaterThan(buffer.getSample(0, numSamples - 1), 0.8f);

            logMessage("  [PASS] Loud signal passes through once the gate is open");
        }

        beginTest("Signal below threshold is attenuated");
        {
            GateProcessor gate(1);
            gate.setThresholdDb(-10.0f);
            gate.setAttackMs(1.0f);
            gate.setReleaseMs(5.0f);   // fast release so it closes quickly for this test
            gate.prepareToPlay(44100.0, 512);

            const int numSamples = 3000; // enough for the gate to close down after release time
            juce::AudioBuffer<float> buffer(1, numSamples);
            // Quiet signal, well below -10dB threshold (-10dB ≈ 0.316 linear; use 0.01 to be clearly under)
            for (int i = 0; i < numSamples; ++i)
                buffer.setSample(0, i, 0.01f);

            juce::MidiBuffer midi;
            gate.processBlock(buffer, midi);

            // Check the tail end, after release has had time to close the gate
            expectLessThan(std::abs(buffer.getSample(0, numSamples - 1)), 0.005f);

            logMessage("  [PASS] Quiet signal is attenuated once the gate closes");
        }

        beginTest("Silence stays silent");
        {
            GateProcessor gate(1);
            gate.setThresholdDb(-40.0f);
            gate.prepareToPlay(44100.0, 512);

            juce::AudioBuffer<float> buffer(1, 4);
            buffer.clear(); // all zeros

            juce::MidiBuffer midi;
            gate.processBlock(buffer, midi);

            for (int i = 0; i < 4; ++i)
                expectWithinAbsoluteError(buffer.getSample(0, i), 0.0f, 0.0001f);

            logMessage("  [PASS] Silent input remains silent regardless of gate state");
        }
    }
};

static GateProcessorTests gateProcessorTests;