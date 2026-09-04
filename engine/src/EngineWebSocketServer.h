#pragma once
#include <boost/asio.hpp>
#include "WsSession.h"
#include "RoutingGraph.h"
#include <thread>
#include <set>
#include <mutex>

class EngineWebSocketServer
{
public:
    explicit EngineWebSocketServer(RoutingGraph& graphToUse, unsigned short port = 9001)
        : routingGraph(graphToUse),
          acceptor(ioc, tcp::endpoint(tcp::v4(), port))
    {}

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

    // Send a message to every connected client — for your Dashboard's
    // levels/deviceList/status pushes.
    void broadcast(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        for (auto& session : sessions)
            session->send(message);
    }

private:
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

                    {
                        std::lock_guard<std::mutex> lock(sessionsMutex);
                        sessions.insert(session);
                    }

                    session->run();
                }

                doAccept(); // keep accepting the next connection
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