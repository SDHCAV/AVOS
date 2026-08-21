//wrapper class for AudioProcessorGraph

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class RoutingGraph
{
    public:
        using NodeID = juce::AudioProcessorGraph::NodeID;

        RoutingGraph();

        //prepare = match graph format to audio device
        void prepare(double sampleRate, 
            int blockSize, 
            int numInputChannels, 
            int numOutputChannels
        );

        NodeID addNode(
            std::unique_ptr<juce::AudioProcessor> processor
        );
        
        //connect specific output channel to node of specific input channel
        //false: JUCE rejects connections (DNE/creates cycle)
        bool connect(NodeID from,
            int fromChannel,
            NodeID to,
            int toChannel    
        );

        //remove specific connection
        //false: DNE
        bool disconnect(NodeID from,
            int fromChannel,
            NodeID to,
            int toChannel

        );

        //engine's audio callback per block
        void processBlock(juce::AudioBuffer<float>& buffer, 
            juce::MidiBuffer& midiMessages);

    NodeID getAudioInputNodeID()  const { return audioInputNode; }
    NodeID getAudioOutputNodeID() const { return audioOutputNode; }

    private:
        juce::AudioProcessorGraph graph;
        NodeID audioInputNode;
        NodeID audioOutputNode;

};