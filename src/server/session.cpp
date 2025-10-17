#include "session.hpp"
#include "server.hpp"

Session::Session(Server& server, size_t id, tcp::socket socket)
    : server_(server),
      sessionId_(id),
      webSocket_(std::move(socket))
{}

size_t Session::getSessionId() const {
    return sessionId_;
}

asio::awaitable<void> Session::start() {
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

asio::awaitable<void> Session::send(const std::string& message) {
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

void Session::close() {
    beast::error_code ec;
    webSocket_.close(websocket::close_code::normal, ec);
    if (ec) {
        std::cerr << "Close error: " << ec.message() << "\n";
    }
}

asio::awaitable<void> Session::readLoop() {
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