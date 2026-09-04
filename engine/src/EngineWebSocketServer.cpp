#include "EngineWebSocketServer.h"
#include <juce_core/juce_core.h>

void EngineWebSocketServer::handleMessage(const std::string& raw)
{
    auto json = juce::JSON::parse(juce::String(raw));

    if (!json.isObject())
    {
        DBG("Received malformed JSON: " << raw);
        return;
    }

    auto type = json["type"].toString();

    if (type == "connect")
    {
        int fromUid = (int) json["from"];
        int fromChannel = (int) json["fromChannel"];
        int toUid = (int) json["to"];
        int toChannel = (int) json["toChannel"];

        RoutingGraph::NodeID from { juce::AudioProcessorGraph::NodeID((uint32_t) fromUid) };
        RoutingGraph::NodeID to   { juce::AudioProcessorGraph::NodeID((uint32_t) toUid) };

        bool ok = routingGraph.connect(from, fromChannel, to, toChannel);
        DBG("connect message: " << (ok ? "OK" : "FAILED"));
    }
    else
    {
        DBG("Unknown message type: " << type);
    }
}