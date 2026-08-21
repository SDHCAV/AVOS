#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class PannerProcessor : public juce::AudioProcessor{
	public:
		PannerProcessor()
			: juce::AudioProcessor(BusesProperties()
				.withInput("Input", juce::AudioChannelSet::mono(), true) //mono() or stereo()
				.withOutput("Output", juce::AudioChannelSet::stereo(), true)
			){}
            //panner = take single mono signal, spread it accross 2 channels
			
			void prepareToPlay(double, int) override {}
			void releaseResources() override {}
			
			void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override{
				const float p = panner.load();

                const float leftGain = (1.0f - p) * 0.5f;
                const float rightGain = (1.0f + p) * 0.5f;

                auto* monoIn = buffer.getReadPointer(0); //incoming = channel 0

                //copy mono signal to its own smaller buffer to write it to output channels
                //so that it doesnt point to the same array, uses independent copies
                juce::AudioBuffer<float> monoCopy(1, buffer.getNumSamples());
                monoCopy.copyFrom(0,0, monoIn, buffer.getNumSamples());
                auto* monoData = monoCopy.getReadPointer(0);

                //pointers for both output channels
                auto* left = buffer.getWritePointer(0);
                auto* right = buffer.getWritePointer(1);

                //for ea sample in block, scale untouched mono source by channel's gain & write it left/right
                for(int sample = 0; sample < buffer.getNumSamples(); sample++){
                    left[sample] = monoData[sample] * leftGain;
                    right[sample] = monoData[sample] * rightGain;
                }
			}
			
			void setPanner(float newPanner){
				panner = juce::jlimit(-1.0f, 1.0f, newPanner);
			}
			const juce::String getName() const override{
				return "Panner";
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
				return{};
			}
			
			void changeProgramName(int, const juce::String&) override{}
			void getStateInformation(juce::MemoryBlock&) override{}
			void setStateInformation(const void*, int) override{}
			
		private:
			std::atomic<float> panner{
				0.0f
			};
	};