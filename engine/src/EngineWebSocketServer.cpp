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

        if (!fromOk || !toOk)
        {
            DBG("connect message: FAILED — unknown endpoint name(s): "
                << (!fromOk ? juce::String(fromName) + " [UNRESOLVED] " : "")
                << (!toOk ? juce::String(toName) + " [UNRESOLVED]" : ""));
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

// bool EngineWebSocketServer::resolveEndpoint(const std::string& name, 
//                                            RoutingGraph::NodeID& nodeOut, 
//                                            int& channelOut)
// {
//     // === EDIT HERE: Hardcode string mappings or query your endpoint map ===
//     if (name == "mixminus-1")
//     {
//         nodeOut = mixMinusNodeID; // Assign your MixMinusBus NodeID
//         channelOut = 0;           // Output channel 0
//         return true;
//     }
//     else if (name == "zoomSend-1")
//     {
//         nodeOut = routingGraph.getAudioOutputNodeID(); // Hardware/Virtual Cable Output
//         channelOut = 1;                               // Channel 1 for Zoom
//         return true;
//     }

//     // If using a dynamic map, replace the above if/else with:
//     /*
//     auto it = endpointMap.find(name);
//     if (it != endpointMap.end())
//     {
//         nodeOut = it->second.nodeID;
//         channelOut = it->second.channel;
//         return true;
//     }
//     */

//     return false; // Name not recognized
// }