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
        // CHANGED: "from"/"to" are now string names (per the spec), not raw integer IDs
        std::string fromName = json["from"].toString().toStdString();
        std::string toName   = json["to"].toString().toStdString();

        RoutingGraph::NodeID fromNode, toNode;
        int fromChannel, toChannel;

        bool fromOk = resolveEndpoint(fromName, fromNode, fromChannel);
        bool toOk   = resolveEndpoint(toName, toNode, toChannel);

        if (!fromOk || !toOk)
        {
            DBG("connect message: FAILED — unknown endpoint name(s): "
                << (fromOk ? "" : juce::String(fromName) + " ")
                << (toOk ? "" : juce::String(toName)));
            return;
        }

        bool ok = routingGraph.connect(fromNode, fromChannel, toNode, toChannel);
        DBG("connect message: " << (ok ? "OK" : "FAILED") 
            << " (" << fromName << " -> " << toName << ")");
    }
    else
    {
        DBG("Unknown message type: " << type);
    }
}