#include "RoutingGraph.h"

//constructor
RoutingGraph::RoutingGraph(){

    //audio in node reads from physical or virtual input dev
    //audio out node writes to physical or virtual ouput device
    auto inputNode = graph.addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode
        )
    );

    auto outputNode = graph.addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode
        )
    );

    audioInputNode = inputNode->nodeID;
    audioOutputNode = outputNode->nodeID;  
}

void RoutingGraph::prepare(double sampleRate, int blockSize, int numInputChannels, int numOutputChannels){
    graph.setPlayConfigDetails(numInputChannels, numOutputChannels, sampleRate, blockSize);
    graph.prepareToPlay(sampleRate, blockSize);
}

RoutingGraph::NodeID RoutingGraph::addNode(std::unique_ptr<juce::AudioProcessor> processor){
    //return Node prt, pull nodeID
    auto node = graph.addNode(std::move(processor));
    return node->nodeID;
}

bool RoutingGraph::connect(NodeID from, int fromChannel, NodeID to, int toChannel){
    //Connection = 2 NodeAndChannel pairs
    juce::AudioProcessorGraph::Connection connection{
        {from, fromChannel},
        {to, toChannel}
    };

    if (!graph.canConnect(connection))
        return false;

    return graph.addConnection(connection);
}

bool RoutingGraph::disconnect(NodeID from, int fromChannel, NodeID to, int toChannel){
    juce::AudioProcessorGraph::Connection connection{
        {from, fromChannel},
        {to, toChannel}
    };

    return graph.removeConnection(connection);
}

//run every node's DSP & moves aydio along all current connections
void RoutingGraph::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages){
    graph.processBlock(buffer, midiMessages);
}
