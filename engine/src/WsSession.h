#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <functional>
#include <string>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class WsSession : public std::enable_shared_from_this<WsSession>
{
public:
    using MessageHandler = std::function<void(std::shared_ptr<WsSession>, const std::string&)>;
    using CloseHandler   = std::function<void(std::shared_ptr<WsSession>)>;

    explicit WsSession(tcp::socket&& socket)
        : ws(std::move(socket))
    {}

    void setMessageHandler(MessageHandler handler) { onMessage = std::move(handler); }
    void setCloseHandler(CloseHandler handler)      { onClose = std::move(handler); }

    void run()
    {
        // Perform the WebSocket handshake asynchronously
        ws.async_accept(
            beast::bind_front_handler(&WsSession::onAccept, shared_from_this()));
    }

    void send(const std::string& message)
    {
        net::post(ws.get_executor(),
            beast::bind_front_handler(&WsSession::doWrite, shared_from_this(), message));
    }

    void close()
    {
        ws.async_close(websocket::close_code::normal,
            [self = shared_from_this()](beast::error_code) {});
    }

private:
    void onAccept(beast::error_code ec)
    {
        if (ec) return; // handshake failed — connection just drops
        doRead();
    }

    void doRead()
    {
        ws.async_read(buffer,
            beast::bind_front_handler(&WsSession::onRead, shared_from_this()));
    }

    void onRead(beast::error_code ec, std::size_t)
    {
        if (ec == websocket::error::closed)
        {
            if (onClose) onClose(shared_from_this());
            return;
        }
        if (ec) return;

        std::string message = beast::buffers_to_string(buffer.data());
        buffer.consume(buffer.size());

        if (onMessage) onMessage(shared_from_this(), message);

        doRead(); // queue up the next read
    }

    void doWrite(const std::string& message)
    {
        // Copy into a member so it stays alive across the async call
        outgoing = message;
        ws.async_write(net::buffer(outgoing),
            beast::bind_front_handler(&WsSession::onWrite, shared_from_this()));
    }

    void onWrite(beast::error_code ec, std::size_t)
    {
        // Nothing to do here for a single message; a real send-queue
        // would pop the next pending message here if you ever
        // need to send faster than one-at-a-time.
    }

    websocket::stream<tcp::socket> ws;
    beast::flat_buffer buffer;
    std::string outgoing;
    MessageHandler onMessage;
    CloseHandler onClose;
};