#include "server.hpp"

#include "pch.hpp"

void Server::run() {
    try {
        asio::signal_set signals(ioContext_, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code& ec, int signal_number) {
            if (ec)
                std::cerr << "Signal handler error: " << ec.message() << "\n";
            std::cout << "\nShutting down server due to signal " << signal_number << "...\n";
            closeAllSessions();
            ioContext_.stop();
        });

        asio::co_spawn(ioContext_, startListener(address_, port_), asio::detached);

        ioContext_.run();

        std::cout << "Server stopped.\n";
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
    }
}

asio::awaitable<void> Server::startListener(std::string_view address, const int port) {
    try {
        auto executor = co_await asio::this_coro::executor;
        const auto endpoint = tcp::endpoint(asio::ip::make_address(address), port);
        std::cout << "Info: Starting listener on " << address << ":" << port << "\n";

        tcp::acceptor acceptor(executor);

        // Open the acceptor
        acceptor.open(endpoint.protocol());

        // Allow address reuse
        acceptor.set_option(asio::socket_base::reuse_address(true));

        // Bind to the server address
        acceptor.bind(endpoint);

        acceptor.listen(asio::socket_base::max_listen_connections);

        for (;;) {
            auto socket = co_await acceptor.async_accept(asio::use_awaitable);

            // Create new session
            size_t sessionId = nextSessionId_++;
            auto session = std::make_shared<Session>(*this, sessionId, std::move(socket));

            // Add to sessions map on the strand
            asio::post(strand_, [this, sessionId, session]() { sessions_[sessionId] = session; });

            std::cout << "Info: New session created: " << sessionId << "\n";

            // Start session in its own coroutine
            asio::co_spawn(executor, session->start(), asio::detached);
        }
    } catch (const boost::system::system_error& se) {
        std::cerr << "Listener error: " << formatWebSocketError(se.code()) << "\n";
    }

    std::cout << "Listener stopped.\n";
    co_return;
}

void Server::removeSession(size_t sessionId) {
    asio::post(strand_, [this, sessionId]() {
        auto it = sessions_.find(sessionId);
        if (it != sessions_.end()) {
            std::cout << "Info: Removing session " << sessionId << "\n";
            sessions_.erase(it);
        }
    });
}

asio::awaitable<void> Server::broadcast(const std::string& message) {
    // Copy sessions on the strand
    auto sessionsCopy = co_await asio::co_spawn(
        strand_,
        [this]() -> asio::awaitable<std::vector<std::shared_ptr<Session>>> {
            std::vector<std::shared_ptr<Session>> result;
            result.reserve(sessions_.size());
            for (const auto& [id, session] : sessions_) {
                result.push_back(session);
            }
            co_return result;
        }(),
        asio::use_awaitable);

    // Send to all sessions
    for (auto& session : sessionsCopy) {
        try {
            co_await session->send(message);
        } catch (...) {
            // Session will be removed by its own error handling
        }
    }
}

asio::awaitable<void> Server::broadcastExcept(const size_t excludeId, const std::string& message) {
    // Copy sessions on the strand
    auto sessionsCopy = co_await asio::co_spawn(
        strand_,
        [this, excludeId]() -> asio::awaitable<std::vector<std::shared_ptr<Session>>> {
            std::vector<std::shared_ptr<Session>> result;
            result.reserve(sessions_.size());
            for (const auto& [id, session] : sessions_) {
                if (id != excludeId) {
                    result.push_back(session);
                }
            }
            co_return result;
        }(),
        asio::use_awaitable);

    for (auto& session : sessionsCopy) {
        try {
            co_await session->send(message);
        } catch (...) {
            // Session will be removed by its own error handling
        }
    }
}

asio::awaitable<void> Server::sendToSession(const size_t sessionId, const std::string& message) {
    // Get session on the strand
    auto session = co_await asio::co_spawn(
        strand_,
        [this, sessionId]() -> asio::awaitable<std::shared_ptr<Session>> {
            auto it = sessions_.find(sessionId);
            if (it != sessions_.end()) {
                co_return it->second;
            }
            co_return nullptr;
        }(),
        asio::use_awaitable);

    if (session) {
        try {
            co_await session->send(message);
        } catch (...) {
            // Session will be removed by its own error handling
        }
    }
}

size_t Server::getSessionCount() const {
    // For a const method, we need to be careful with strand access
    // This is a read operation but sessions_ can change, so still needs synchronization
    // Using atomic or returning approximate count is acceptable for monitoring
    return sessions_.size();  // Note: This is an approximate count without strand protection
}

void Server::closeAllSessions() {
    asio::post(strand_, [this]() {
        for (auto& [id, session] : sessions_) {
            session->close();
        }
        sessions_.clear();
    });
}
