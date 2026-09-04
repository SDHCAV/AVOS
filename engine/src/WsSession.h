#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <functional>
#include <string>
#include <deque>   // CHANGED: needed for the write queue

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class WsSession : public std::enable_shared_from_this<WsSession>
{
public:
    using MessageHandler = std::function<void(std::shared_ptr<WsSession>, const std::string&)>;
    using CloseHandler   = std::function<void(std::shared_ptr<WsSession>)>;
    using OpenHandler    = std::function<void(std::shared_ptr<WsSession>)>;   // CHANGED: new — fires once handshake truly completes

    explicit WsSession(tcp::socket&& socket)
        : ws(std::move(socket))
    {}

    void setMessageHandler(MessageHandler handler) { onMessage = std::move(handler); }
    void setCloseHandler(CloseHandler handler)      { onClose = std::move(handler); }
    void setOpenHandler(OpenHandler handler)        { onOpen = std::move(handler); }   // CHANGED: new setter

    void run()
    {
        ws.async_accept(
            beast::bind_front_handler(&WsSession::onAccept, shared_from_this()));
    }

    void send(const std::string& message)
    {
        net::post(ws.get_executor(),
            beast::bind_front_handler(&WsSession::enqueueWrite, shared_from_this(), message));
        // CHANGED: now posts to enqueueWrite() instead of doWrite() directly,
        // so overlapping sends get queued instead of colliding
    }

    void close()
    {
        ws.async_close(websocket::close_code::normal,
            [self = shared_from_this()](beast::error_code) {});
    }

private:
    void onAccept(beast::error_code ec)
    {
        if (ec) return;
        if (onOpen) onOpen(shared_from_this());   // CHANGED: notify that the handshake is genuinely done —
                                                     // this is the correct place to send a "welcome" message like deviceList
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

        doRead();
    }

    // CHANGED: replaced the old single-message doWrite(std::string) with a queue-based version.
    // Beast forbids starting a new async_write before the previous one's handler has fired,
    // so multiple sends in quick succession (e.g. broadcastLevels sending 2 messages back to back)
    // need to be serialized rather than fired concurrently.

    void enqueueWrite(const std::string& message)
    {
        writeQueue.push_back(message);
        if (writeQueue.size() == 1)   // nothing currently in flight — start sending now
            doWrite();
        // if queue was already non-empty, onWrite() will pick this up when it's this item's turn
    }

    void doWrite()
    {
        ws.async_write(net::buffer(writeQueue.front()),
            beast::bind_front_handler(&WsSession::onWrite, shared_from_this()));
    }

    void onWrite(beast::error_code ec, std::size_t)
    {
        writeQueue.pop_front();
        if (ec) return;
        if (!writeQueue.empty())
            doWrite();   // send whatever's next in line
    }

    websocket::stream<tcp::socket> ws;
    beast::flat_buffer buffer;
    std::deque<std::string> writeQueue;   // CHANGED: replaces the old single `std::string outgoing;`
    MessageHandler onMessage;
    CloseHandler onClose;
    OpenHandler onOpen;   // CHANGED: new member
};