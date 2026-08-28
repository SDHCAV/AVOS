#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

class MixMinusBus : public juce::AudioProcessor{
	public: 
		explicit MixMinusBus(int numInputs);
		
		void prepareToPlay(double sampleRate, int samplesPerBlock) override;
		void releaseResources() override;
		void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
		
        //which input channel index should be left out of the sum
        //-1 to exclude nothing (sums everything)
		void setExcludedChannel(int channelIndex);
		
        const juce::String getName() const override {
			return "MixMinusBus";
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
			return 0;
		}
		void setCurrentProgram(int) override{}
		const juce::String getProgramName(int) override{
			return {};
		}
		void changeProgramName(int, const juce::String&) override{}
		void getStateInformation(juce::MemoryBlock&) override{}
		void setStateInformation(const void*, int) override {}
		
	private:
		static juce::AudioProcessor::BusesProperties makeBusLayout(int numInputs);
		
		int numInputChannels;
		std::atomic<int> excludedChannel{
			-1
		};
		
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixMinusBus)
	};