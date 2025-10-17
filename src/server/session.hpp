#pragma once
#include "pch.hpp"
#include "common/helpers.hpp"

class Server;

class Session : public std::enable_shared_from_this<Session>
{
    Server& server_;
    size_t sessionId_;
    websocket::stream<tcp::socket> webSocket_;
    beast::flat_buffer buffer_;

public:
    Session(Server& server, size_t id, tcp::socket socket);
    ~Session() = default;

    size_t getSessionId() const;
    asio::awaitable<void> start();
    asio::awaitable<void> send(const std::string& message);
    void close();

private:
    asio::awaitable<void> readLoop();
    asio::awaitable<void> handleMessage(const std::string& message);
    void removeFromServer();
};
