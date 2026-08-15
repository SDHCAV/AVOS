#pragma once
#include <juce_audio_processors/juice_audio_processors.h>

class GainProcessor : public juce::AudioProcessor{
    public:
        GainProccessor();

        void prepareToPlay(double sanpleRate, int samplesPerBlock) override;
        void releaseResources() override;
        void processBlock(juce::AudiBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

        void setGain(float newGain) {
            gain = newGain;
        }
        float getGain() const{
            return gain;
        }

        const juce::String getName() const override{
            return "Gain";
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
        int getNumProfeams() override {
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
        void setStateInformation(const void*, int) override{}

    private:
        std::atomic<float> gain {
            1.0f
        };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainProcessor)
}