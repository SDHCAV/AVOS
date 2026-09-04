#pragma once
#include <boost/asio.hpp>
#include "WsSession.h"
#include "RoutingGraph.h"
#include <thread>
#include <set>
#include <mutex>
#include <juce_core/juce_core.h>   // CHANGED: needed here now for juce::StringArray / JSON building

class EngineWebSocketServer
{
public:
    using DeviceListProvider = std::function<juce::StringArray()>;

    explicit EngineWebSocketServer(RoutingGraph& graphToUse, unsigned short port = 9001)
        : routingGraph(graphToUse),
          acceptor(ioc, tcp::endpoint(tcp::v4(), port))
    {}

    void setDeviceListProvider(DeviceListProvider provider)
    {
        deviceListProvider = std::move(provider);
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

                    // CHANGED: new — this replaces the old "send deviceList right after run()" code.
                    // onOpen only fires once the WebSocket handshake has genuinely completed,
                    // so it's safe to send here (unlike right after calling run(), which is async
                    // and returns before the handshake is actually done).
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

                                juce::String json = juce::JSON::toString(juce::var(obj.get()));
                                s->send(json.toStdString());
                            }
                        });

                    {
                        std::lock_guard<std::mutex> lock(sessionsMutex);
                        sessions.insert(session);
                    }

                    session->run();
                    // CHANGED: no longer sending deviceList right here — moved into setOpenHandler above
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
};