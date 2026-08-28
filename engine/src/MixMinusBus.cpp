#include "MixMinusBus.h"

juce::AudioProcessor::BusesProperties MixMinusBus::makeBusLayout(int numInputs){
	//build n input buses in a loop
	//count isnt fixed like stereo() or mono()
	//ea source gets its own mono bus (named by index)
	BusesProperties layout;
	for(int i = 0; i < numInputs; i++)
		layout = layout.withInput("Input " + juce::String(i), juce::AudioChannelSet::mono(), true);
		
	//single output bus: combined/processed result
	layout = layout.withOutput("Output", juce::AudioChannelSet::mono(), true);
	return layout;
}

MixMinusBus::MixMinusBus(int numInputs)
	:juce::AudioProcessor(makeBusLayout(numInputs)),
		numInputChannels(numInputs)
{
}

void MixMinusBus::prepareToPlay(double, int){}
void MixMinusBus::releaseResources(){}

void MixMinusBus::setExcludedChannel(int channelIndex){
	excludedChannel = channelIndex;
}

void MixMinusBus::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&){
	const int excluded = excludedChannel.load();
	const int numSamples = buffer.getNumSamples();
	
	//work in separate buffer so we dont read from channel we are simultaneiouslt accumulating into
	//same reason as PannerProcessor's monoCopy
	//reading & writing same array mid-loop corrupts data
	juce::AudioBuffer<float> sumBuffer(1, numSamples);
	sumBuffer.clear();
	
	for(int channel = 0; channel < numInputChannels; channel++){
		//bus specific inclusion/exclusion logic HERE
		//mixminus: if channel == flagged channel, continue (SKIP IT)
		//solo bus: if channel not == flagged channel, continue (KEEP ONLY IT)
		//fill in based on what bus is supposed to do
        if (channel == excluded)
            continue;
        sumBuffer.addFrom(0,0, buffer, channel, 0, numSamples);
	}
	
	buffer.copyFrom(0, 0, sumBuffer, 0, 0, numSamples);
}