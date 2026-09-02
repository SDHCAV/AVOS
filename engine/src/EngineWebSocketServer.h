#pragma once
#include <server_ws.hpp>
#include <juce_core/juce_core.h>
#include "RoutingGraph.h"
#include <thread>

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;

class EngineWebSocketServer
{
public:
    explicit EngineWebSocketServer(RoutingGraph& graphToUse)
        : routingGraph(graphToUse)
    {
        server.config.port = 9001;

        auto& endpoint = server.endpoint["^/$"];

        endpoint.on_message = [this](std::shared_ptr<WsServer::Connection> connection,
                                      std::shared_ptr<WsServer::Message> message)
        {
            handleMessage(message->string());
        };

        endpoint.on_open = [](std::shared_ptr<WsServer::Connection>)
        {
            DBG("WebSocket client connected.");
        };
    }

    void start()
    {
        serverThread = std::thread([this] { server.start(); });
    }

    void stop()
    {
        server.stop();
        if (serverThread.joinable())
            serverThread.join();
    }

private:
    void handleMessage(const std::string& raw);

    WsServer server;
    RoutingGraph& routingGraph;
    std::thread serverThread;
};