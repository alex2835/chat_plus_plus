#pragma once
#include "pch.hpp"

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

asio::awaitable<void> handleSession(tcp::socket socket) {
    try {
        beast::flat_buffer buffer;

        // First handle HTTP Upgrade
        http::request<http::string_body> request;
        co_await http::async_read(socket, buffer, request, asio::use_awaitable);

        // Verify it's a WebSocket upgrade request
        if (!websocket::is_upgrade(request)) {
            std::cerr << "Error: Not a WebSocket upgrade request\n";
            co_return;
        }

        websocket::stream<tcp::socket> webSocket(std::move(socket));
        
        // Set timeout and server decorator
        webSocket.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
        
        // Accept the websocket handshake
        co_await webSocket.async_accept(request, asio::use_awaitable);
        std::cout << "Info: WebSocket connection established\n";
        
        for (;;) {
            try {
                // Read a message
                co_await webSocket.async_read(buffer, asio::use_awaitable);
                
                std::string messageText = beast::buffers_to_string(buffer.data());
                std::cout << "Message: " << messageText << "\n";
                
                // Echo back with prefix
                std::string responseText = "Echo: " + messageText;
                webSocket.text(webSocket.got_text());
                co_await webSocket.async_write(asio::buffer(responseText), asio::use_awaitable);
                
                buffer.consume(buffer.size());
            }
            catch (boost::system::system_error& se) {
                std::cerr << "WebSocket: " << formatWebSocketError(se.code()) << "\n";
                break;
            }
        }
    } catch (const boost::system::system_error& se) {
        std::cerr << "Session error: " << formatWebSocketError(se.code()) << "\n";
    } catch (const std::exception& e) {
        std::cerr << "General error: " << e.what() << "\n";
    }
}

asio::awaitable<void> startListener(std::string_view address, std::string_view port)
{
    try {
        auto executor = co_await asio::this_coro::executor;
        const auto endpoint = tcp::endpoint(asio::ip::make_address(address), std::stoi(std::string(port)));
        std::cout << "Info: Starting listener on " << address << ":" << port << std::endl;

        beast::error_code errorCode;
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
            asio::co_spawn(executor, handleSession(std::move(socket)), asio::detached);
        }
    }
    catch (boost::system::system_error& se) {
        std::cerr << "WebSocket: " << formatWebSocketError(se.code()) << "\n";
        co_return;
    }
    std::cout << "Listener stopped." << std::endl;
    co_return;
}

void startServer(std::string_view address, std::string_view port) {
    try {
        asio::io_context ioContext;
        
        asio::signal_set signals(ioContext, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) {
            ioContext.stop();
        });

        asio::co_spawn(ioContext, startListener(address, port), asio::detached);

        ioContext.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
    }
}
