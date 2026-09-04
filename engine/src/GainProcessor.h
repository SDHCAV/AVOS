#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class GainProcessor : public juce::AudioProcessor{
    public:
        GainProcessor()
            : juce::AudioProcessor(BusesProperties()
                .withInput("Input", juce::AudioChannelSet::stereo(), true)
                .withOutput("Output", juce::AudioChannelSet::stereo(), true)
        ){

        }

        void prepareToPlay(double, int) override {}
        void releaseResources() override {}
        void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override{
            buffer.applyGain(gain.load());
        }

        void setGain(float newGain) {
            gain = newGain;
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
        int getNumPrograms() override {
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
};