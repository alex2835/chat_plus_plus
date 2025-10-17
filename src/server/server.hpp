#pragma once
#include "pch.hpp"
#include "common/helpers.hpp"
#include "session.hpp"

class Server
{
private:
    asio::io_context ioContext_;
    asio::strand<asio::io_context::executor_type> strand_;
    std::string_view address_;
    int port_;
    std::unordered_map<size_t, std::shared_ptr<Session>> sessions_;
    std::atomic<size_t> nextSessionId_{0};

public:
    Server(std::string_view address, int port, int threads = 1)
        : ioContext_(threads),
          strand_(asio::make_strand(ioContext_)),
          address_(address),
          port_(port)
    {}

    void run();
    asio::awaitable<void> startListener(std::string_view address, const int port);
    void removeSession(size_t sessionId);

    asio::awaitable<void> broadcast(const std::string& message);
    asio::awaitable<void> broadcastExcept(const size_t excludeId, const std::string& message);

    asio::awaitable<void> sendToSession(const size_t sessionId,const std::string& message);
    size_t getSessionCount() const;
    void closeAllSessions();
};

