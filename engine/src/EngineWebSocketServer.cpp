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
        std::string fromName = json["from"].toString().toStdString();
        std::string toName   = json["to"].toString().toStdString();

        RoutingGraph::NodeID fromNode, toNode;
        int fromChannel, toChannel;

        bool fromOk = resolveEndpoint(fromName, fromNode, fromChannel);
        bool toOk   = resolveEndpoint(toName, toNode, toChannel);

        DBG("connect attempt: from=" << fromName << " resolved=" << (fromOk ? "yes" : "no")
            << " node=" << (int) fromNode.uid << " channel=" << fromChannel);
        DBG("connect attempt: to=" << toName << " resolved=" << (toOk ? "yes" : "no")
            << " node=" << (int) toNode.uid << " channel=" << toChannel);

        bool ok = false;
        if (fromOk && toOk)
            ok = routingGraph.connect(fromNode, fromChannel, toNode, toChannel);

        DBG("connect message: " << (ok ? "OK" : "FAILED") << " (" << fromName << " -> " << toName << ")");
        sendConnectAck(fromName, toName, ok);
    }
    else if (type == "disconnect")   // CHANGED: new — mirrors "connect"
    {
        std::string fromName = json["from"].toString().toStdString();
        std::string toName   = json["to"].toString().toStdString();

        RoutingGraph::NodeID fromNode, toNode;
        int fromChannel, toChannel;

        bool fromOk = resolveEndpoint(fromName, fromNode, fromChannel);
        bool toOk   = resolveEndpoint(toName, toNode, toChannel);

        bool ok = false;
        if (fromOk && toOk)
            ok = routingGraph.disconnect(fromNode, fromChannel, toNode, toChannel);

        DBG("disconnect message: " << (ok ? "OK" : "FAILED")
            << " (" << fromName << " -> " << toName << ")");

        sendConnectAck(fromName, toName, ok);   // reuses the same ack shape — client already handles "connect" type acks
    }
    else if (type == "getEndpoints")   // CHANGED: new — client-requested snapshot
    {
        // find the requesting session and reply directly — but handleMessage
        // currently doesn't have access to which session sent this message.
        // Simplest correct fix: just broadcast it, since every client wants
        // the same snapshot anyway, and it's cheap.
        broadcast(buildEndpointsMessage());
    }
    else
    {
        DBG("Unknown message type: " << type);
    }
}