#pragma once
#include "pch.hpp"
#include <unordered_map>
#include <memory>
#include <atomic>

// Helper function to format WebSocket errors
std::string formatWebSocketError(const boost::system::error_code& errorCode) {
    if (errorCode == websocket::error::closed) {
        return "WebSocket closed normally";
    }
    if (errorCode == boost::asio::error::connection_reset) {
        return "Connection reset by client";
    }
    if (errorCode == boost::asio::error::connection_aborted) {
        return "Connection aborted";
    }
    if (errorCode == boost::asio::error::timed_out) {
        return "Connection timed out";
    }
    // Extract just the error message without the long location info
    std::string errorMsg = errorCode.message();
    size_t pos = errorMsg.find(" [");
    if (pos != std::string::npos) {
        errorMsg = errorMsg.substr(0, pos);
    }
    return errorMsg;
}

// Forward declaration
class Server;

class Session : public std::enable_shared_from_this<Session>
{
    Server& server_;
    size_t sessionId_;
    websocket::stream<tcp::socket> webSocket_;
    beast::flat_buffer buffer_;

public:
    Session(Server& server, size_t id, tcp::socket socket)
        : server_(server),
          sessionId_(id),
          webSocket_(std::move(socket))
    {}

    size_t getSessionId() const {
        return sessionId_;
    }

    asio::awaitable<void> start() {
        try {
            // Set timeout and server decorator
            webSocket_.set_option(
                websocket::stream_base::timeout::suggested(beast::role_type::server)
            );
            
            // Accept the websocket handshake
            co_await webSocket_.async_accept(asio::use_awaitable);
            std::cout << "Info: Session " << sessionId_ << " connected\n";
            
            // Start reading messages
            co_await readLoop();
            
        } catch (const boost::system::system_error& se) {
            std::cerr << "Session " << sessionId_ << " error: " 
                      << formatWebSocketError(se.code()) << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Session " << sessionId_ << " error: " << e.what() << "\n";
        }
        
        // Clean up when session ends
        removeFromServer();
    }

    asio::awaitable<void> send(const std::string& message) {
        try {
            co_await webSocket_.async_write(
                asio::buffer(message), 
                asio::use_awaitable
            );
        } catch (const boost::system::system_error& se) {
            std::cerr << "Send error in session " << sessionId_ << ": " 
                      << formatWebSocketError(se.code()) << "\n";
            throw;
        }
    }

    void close() {
        beast::error_code ec;
        webSocket_.close(websocket::close_code::normal, ec);
        if (ec) {
            std::cerr << "Close error: " << ec.message() << "\n";
        }
    }

private:
    asio::awaitable<void> readLoop() {
        for (;;) {
            try {
                // Read a message
                co_await webSocket_.async_read(buffer_, asio::use_awaitable);
                
                std::string messageText = beast::buffers_to_string(buffer_.data());
                std::cout << "Session " << sessionId_ << " message: " << messageText << "\n";
                
                // Process message
                co_await handleMessage(messageText);
                
                buffer_.consume(buffer_.size());
                
            } catch (const boost::system::system_error& se) {
                if (se.code() != websocket::error::closed) {
                    std::cerr << "Read error in session " << sessionId_ << ": " 
                              << formatWebSocketError(se.code()) << "\n";
                }
                break;
            }
        }
    }

    asio::awaitable<void> handleMessage(const std::string& message);

    void removeFromServer();
};

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

    void run() {
        try {
            asio::signal_set signals(ioContext_, SIGINT, SIGTERM);
            signals.async_wait([&](const boost::system::error_code& ec, int signal_number) {
                if (ec) std::cerr << "Signal handler error: " << ec.message() << "\n";
                std::cout << "\nShutting down server due to signal " << signal_number << "...\n";
                closeAllSessions();
                ioContext_.stop();
            });

            asio::co_spawn(
                ioContext_, 
                startListener(address_, port_), 
                asio::detached
            );

            ioContext_.run();
            
            std::cout << "Server stopped.\n";
        } catch (const std::exception& e) {
            std::cerr << "Server error: " << e.what() << "\n";
        }
    }

    asio::awaitable<void> startListener(std::string_view address, const int port) {
        try {
            auto executor = co_await asio::this_coro::executor;
            const auto endpoint = tcp::endpoint(
                asio::ip::make_address(address), 
                port
            );
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
                auto session = std::make_shared<Session>(
                    *this,
                    sessionId,
                    std::move(socket)
                );
                
                // Add to sessions map on the strand
                asio::post(strand_, [this, sessionId, session]() {
                    sessions_[sessionId] = session;
                });

                std::cout << "Info: New session created: " << sessionId << "\n";

                // Start session in its own coroutine
                asio::co_spawn(
                    executor,
                    session->start(),
                    asio::detached
                );
            }
        } catch (const boost::system::system_error& se) {
            std::cerr << "Listener error: " << formatWebSocketError(se.code()) << "\n";
        }
        
        std::cout << "Listener stopped.\n";
        co_return;
    }

    void removeSession(size_t sessionId) {
        asio::post(strand_, [this, sessionId]() {
            auto it = sessions_.find(sessionId);
            if (it != sessions_.end()) {
                std::cout << "Info: Removing session " << sessionId << "\n";
                sessions_.erase(it);
            }
        });
    }

    asio::awaitable<void> broadcast(const std::string& message) {
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
            asio::use_awaitable
        );

        // Send to all sessions
        for (auto& session : sessionsCopy) {
            try {
                co_await session->send(message);
            } catch (...) {
                // Session will be removed by its own error handling
            }
        }
    }

    asio::awaitable<void> broadcastExcept(
        const size_t excludeId,
        const std::string& message
    ) {
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
            asio::use_awaitable
        );

        for (auto& session : sessionsCopy) {
            try {
                co_await session->send(message);
            } catch (...) {
                // Session will be removed by its own error handling
            }
        }
    }

    asio::awaitable<void> sendToSession(
        const size_t sessionId,
        const std::string& message
    ) {
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
            asio::use_awaitable
        );

        if (session) {
            try {
                co_await session->send(message);
            } catch (...) {
                // Session will be removed by its own error handling
            }
        }
    }

    size_t getSessionCount() const {
        // For a const method, we need to be careful with strand access
        // This is a read operation but sessions_ can change, so still needs synchronization
        // Using atomic or returning approximate count is acceptable for monitoring
        return sessions_.size();  // Note: This is an approximate count without strand protection
    }

    void closeAllSessions() {
        asio::post(strand_, [this]() {
            for (auto& [id, session] : sessions_) {
                session->close();
            }
            sessions_.clear();
        });
    }
};

// Implement Session methods that need Server definition
asio::awaitable<void> Session::handleMessage(const std::string& message) {
    // Echo back with prefix
    std::string responseText = "Echo: " + message;
    webSocket_.text(webSocket_.got_text());
    co_await send(responseText);
    
    // Example: broadcast to all other sessions
    // co_await server_.broadcastExcept(sessionId_, "User sent: " + message);
}

void Session::removeFromServer() {
    server_.removeSession(sessionId_);
}

