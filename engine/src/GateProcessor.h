#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cmath>

class GateProcessor : public juce::AudioProcessor{
	public:
		explicit GateProcessor(int numChannels = 1)
			: juce::AudioProcessor(makeBusLayout(numChannels)
			){}
			
			void prepareToPlay(double sampleRate, int) override {
                //attack/release times control how fast gate open closes
                attackCoeff = calcCoeff(attackMs.load(), sampleRate); //faster = catch transients quicker
                releaseCoeff = calcCoeff(releaseMs.load(), sampleRate); //slower = less clicky cutoff
            }
			void releaseResources() override {}
			
			void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override{
				const float threshold = thresholdLinear.load();
            const int numSamples = buffer.getNumSamples();
            const int numChannels = buffer.getNumChannels();

            for (int sample = 0; sample < numSamples; ++sample)
            {
                // Measure this sample's level across all channels (simple peak, not full RMS —
                // cheap and good enough for a first version)
                float peak = 0.0f;
                for (int ch = 0; ch < numChannels; ++ch)
                    peak = std::max(peak, std::abs(buffer.getReadPointer(ch)[sample]));

                // Decide target gate state: 1.0 = fully open, 0.0 = fully closed
                float target = (peak >= threshold) ? 1.0f : 0.0f;

                // Smooth toward that target using attack (opening) or release (closing) speed
                float coeff = (target > currentGain) ? attackCoeff : releaseCoeff;
                currentGain = target + (currentGain - target) * coeff;

                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.getWritePointer(ch)[sample] *= currentGain;
            }
			}
			
			void setThresholdDb(float db){
                thresholdLinear = juce::Decibels::decibelsToGain(db);
            }

            void setAttackMs(float ms) { attackMs = ms; }
            void setReleaseMs(float ms) { releaseMs = ms; }

			const juce::String getName() const override{
				return "Gate";
			}
			double getTailLengthSeconds() const override{
				return 0.0;
			}
			bool acceptsMidi() const override{
				return false;
			}
			bool producesMidi() const override{
				return false;
			}
			juce::AudioProcessorEditor* createEditor() override{
				return nullptr;
			}
			bool hasEditor() const override{
				return false;
			}
			int getNumPrograms() override{
				return 1;
			}
			int getCurrentProgram() override{
				return 1;
			}
			
			void setCurrentProgram(int) override{}
			
			const juce::String getProgramName(int) override{
				return{};
			}
			
			void changeProgramName(int, const juce::String&) override{}
			void getStateInformation(juce::MemoryBlock&) override{}
			void setStateInformation(const void*, int) override{}
			
		private:
			static juce::AudioProcessor::BusesProperties makeBusLayout(int numChannels)
            {
                return juce::AudioProcessor::BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::canonicalChannelSet(numChannels), true)
                    .withOutput("Output", juce::AudioChannelSet::canonicalChannelSet(numChannels), true);
            }

            static float calcCoeff(float timeMs, double sampleRate)
            {
                if (timeMs <= 0.0f) return 0.0f;
                return std::exp(-1.0f / (0.001f * timeMs * (float) sampleRate));
            }

            std::atomic<float> thresholdLinear { juce::Decibels::decibelsToGain(-40.0f) }; // default -40dB
            std::atomic<float> attackMs { 5.0f };    // fast open, ~5ms
            std::atomic<float> releaseMs { 150.0f }; // slower close, ~150ms — avoids abrupt cutoff

            float attackCoeff = 0.0f;
            float releaseCoeff = 0.0f;
            float currentGain = 0.0f;   // starts closed

	};