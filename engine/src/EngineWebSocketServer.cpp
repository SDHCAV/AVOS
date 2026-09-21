#include "EngineWebSocketServer.h"
#include <juce_core/juce_core.h>
#include <iostream>

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

        std::cout << "connect message: " << (ok ? "OK" : "FAILED") << " (" << fromName << " -> " << toName << ")" << std::endl;
        sendConnectAck(fromName, toName, ok);
    }
    else if (type == "disconnect")   
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

        std::cout << "disconnect message: " << (ok ? "OK" : "FAILED")
            << " (" << fromName << " -> " << toName << ")" << std::endl;

        sendConnectAck(fromName, toName, ok);
    }
    else if (type == "getEndpoints")
    {
        broadcast(buildEndpointsMessage());
    }
    else if (type == "setParam")
    {
        std::string node = json["node"].toString().toStdString();
        std::string param = json["param"].toString().toStdString();
        float value = (float) json["value"];

        auto it = paramSetters.find(node + "." + param);
        if (it != paramSetters.end())
        {
            it->second(value);
            std::cout << "setParam OK: " << node << "." << param << " = " << value << std::endl;
        }
        else
        {
            std::cout << "setParam FAILED: unknown " << node << "." << param << std::endl;
        }
    }
    else
    {
        std::cout << "Unknown message type: " << type << std::endl;
    }
}