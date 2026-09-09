#pragma once
#include <boost/asio.hpp>
#include "WsSession.h"
#include "RoutingGraph.h"
#include <thread>
#include <set>
#include <mutex>
#include <map>
#include <string>
#include <juce_core/juce_core.h>

class EngineWebSocketServer
{
public:
    using DeviceListProvider = std::function<juce::StringArray()>;

    // CHANGED: new — lets registerEndpoint tag what kind of thing this is,
    // so internal processing nodes (gain, panner) can be hidden from the UI
    enum class EndpointKind { Source, Destination, Internal };

    struct NamedEndpoint
    {
        RoutingGraph::NodeID node;
        int channel;
        EndpointKind kind;   // CHANGED: added
    };

    explicit EngineWebSocketServer(RoutingGraph& graphToUse, unsigned short port = 9001)
        : routingGraph(graphToUse),
          acceptor(ioc, tcp::endpoint(tcp::v4(), port))
    {}

    void setDeviceListProvider(DeviceListProvider provider)
    {
        deviceListProvider = std::move(provider);
    }

    // CHANGED: signature now takes `kind`
    void registerEndpoint(const std::string& name, RoutingGraph::NodeID node, int channel, EndpointKind kind)
    {
        endpoints[name] = { node, channel, kind };
    }

    bool resolveEndpoint(const std::string& name, RoutingGraph::NodeID& outNode, int& outChannel) const
    {
        auto it = endpoints.find(name);
        if (it == endpoints.end())
            return false;
        outNode = it->second.node;
        outChannel = it->second.channel;
        return true;
    }

    void start()
    {
        doAccept();
        serverThread = std::thread([this] { ioc.run(); });
    }

    void stop()
    {
        ioc.stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    void broadcast(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        for (auto& session : sessions)
            session->send(message);
    }

    void broadcastLevels(const std::string& node, float peak, float rms)
    {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("type", "levels");
        obj->setProperty("node", juce::String(node));
        obj->setProperty("peak", peak);
        obj->setProperty("rms", rms);

        juce::String json = juce::JSON::toString(juce::var(obj.get()));
        broadcast(json.toStdString());
    }

    // CHANGED: new — resolves a NodeID+channel back to its registered name.
    // Option B: linear search, fine given the small number of registered endpoints.
    std::string findNameForEndpoint(RoutingGraph::NodeID node, int channel) const
    {
        for (auto& [name, ep] : endpoints)
        {
            if (ep.node == node && ep.channel == channel)
                return name;
        }
        return {};   // not found — likely an internal/unregistered node
    }

    // CHANGED: new — builds the full "endpoints" message: filtered endpoint list + current connections
    std::string buildEndpointsMessage() const
    {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("type", "endpoints");

        // endpoint list — skip anything tagged Internal
        juce::Array<juce::var> endpointArray;
        for (auto& [name, ep] : endpoints)
        {
            if (ep.kind == EndpointKind::Internal)
                continue;

            juce::DynamicObject::Ptr epObj = new juce::DynamicObject();
            epObj->setProperty("id", juce::String(name));
            epObj->setProperty("label", juce::String(name));   // TODO: nicer display names later if needed
            epObj->setProperty("kind", ep.kind == EndpointKind::Source ? "source" : "destination");
            endpointArray.add(juce::var(epObj.get()));
        }
        obj->setProperty("endpoints", endpointArray);

        // current connections — resolve each NodeID/channel pair back to a registered name;
        // skip any connection where either end isn't a registered (or is an internal) endpoint
        juce::Array<juce::var> connectionArray;
        for (auto& conn : routingGraph.getCurrentConnections())
        {
            std::string fromName = findNameForEndpoint(conn.fromNode, conn.fromChannel);
            std::string toName   = findNameForEndpoint(conn.toNode, conn.toChannel);

            if (fromName.empty() || toName.empty())
                continue;

            auto fromKind = endpoints.at(fromName).kind;
            auto toKind = endpoints.at(toName).kind;
            if (fromKind == EndpointKind::Internal || toKind == EndpointKind::Internal)
                continue;

            juce::DynamicObject::Ptr connObj = new juce::DynamicObject();
            connObj->setProperty("from", juce::String(fromName));
            connObj->setProperty("to", juce::String(toName));
            connectionArray.add(juce::var(connObj.get()));
        }
        obj->setProperty("connections", connectionArray);

        return juce::JSON::toString(juce::var(obj.get())).toStdString();
    }

    // CHANGED: new — broadcasts a connect/disconnect result to all clients
    void sendConnectAck(const std::string& from, const std::string& to, bool ok)
    {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("type", "connect");
        obj->setProperty("ok", ok);
        obj->setProperty("from", juce::String(from));
        obj->setProperty("to", juce::String(to));

        broadcast(juce::JSON::toString(juce::var(obj.get())).toStdString());
    }

private:
    DeviceListProvider deviceListProvider;

    void doAccept()
    {
        acceptor.async_accept(
            [this](beast::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    auto session = std::make_shared<WsSession>(std::move(socket));

                    session->setMessageHandler(
                        [this](std::shared_ptr<WsSession> s, const std::string& msg)
                        {
                            handleMessage(msg);
                        });

                    session->setCloseHandler(
                        [this](std::shared_ptr<WsSession> s)
                        {
                            std::lock_guard<std::mutex> lock(sessionsMutex);
                            sessions.erase(s);
                        });

                    session->setOpenHandler(
                        [this](std::shared_ptr<WsSession> s)
                        {
                            if (deviceListProvider)
                            {
                                auto devices = deviceListProvider();

                                juce::DynamicObject::Ptr obj = new juce::DynamicObject();
                                obj->setProperty("type", "deviceList");

                                juce::Array<juce::var> deviceArray;
                                for (auto& name : devices)
                                    deviceArray.add(name);
                                obj->setProperty("devices", deviceArray);

                                s->send(juce::JSON::toString(juce::var(obj.get())).toStdString());
                            }

                            // CHANGED: also send the current endpoints/connections snapshot on connect —
                            // covers the "server also sends this unprompted on connect" comment in engineSocket.ts
                            s->send(buildEndpointsMessage());
                        });

                    {
                        std::lock_guard<std::mutex> lock(sessionsMutex);
                        sessions.insert(session);
                    }

                    session->run();
                }

                doAccept();
            });
    }

    void handleMessage(const std::string& raw);

    net::io_context ioc{1};
    tcp::acceptor acceptor;
    RoutingGraph& routingGraph;
    std::thread serverThread;

    std::set<std::shared_ptr<WsSession>> sessions;
    std::mutex sessionsMutex;
    std::map<std::string, NamedEndpoint> endpoints;
};