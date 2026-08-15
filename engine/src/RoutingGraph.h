//wrapper class for AudioProcessorGraph

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <unordered_map>

class RoutingGraph
{
    public:        
        using NodeID = juce::AudioProcessorGraph::NodeID;

        RoutingGraph();

        //prepare = match graph format to audio device
        void prepare(double sampleRate, int blockSize, int numInputChannels, int numOutputChannels);

        //add node: wraps processor as graph node
        //returns its ID to reference later for connecting and disconnecting
        NodeID addNode(std::unique_ptr<juce::AudioProcessor> processor);

        //connect specific output channel to node of specific input channel
        //false: JUCE rejects connections (DNE/creates cycle)
        bool connect(NodeID from, int fromChannel, NodeID to, int toChannel);

        //remove specific connection
        //false: DNE
        bool disconnect(NodeID from, int fromChannel, NodeID to, int toChannel);

        //convenience accessors for build in hardware IO nodes
        //set up automatically in constructor
        NodeID getAudioInputNodeID() const{
            return audioInputNodeID;
        }
        NodeID getAudioOutputNodeID() const{
            return audioOutputNodeID;
        }

        //engine's audio callback per block
        void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    private:
        juce::AudioProcessorGraph graph;

        NodeID audioInputNodeID;
        NodeID audioOutputNodeID;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingGraph)
};