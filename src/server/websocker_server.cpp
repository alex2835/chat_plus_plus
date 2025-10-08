#include "pch.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

// Helper function to format WebSocket errors
std::string format_websocket_error(const boost::system::error_code& ec) {
    if (ec == websocket::error::closed) {
        return "WebSocket closed normally";
    }
    if (ec == boost::asio::error::connection_reset) {
        return "Connection reset by client";
    }
    if (ec == boost::asio::error::connection_aborted) {
        return "Connection aborted";
    }
    if (ec == boost::asio::error::timed_out) {
        return "Connection timed out";
    }
    // Extract just the error message without the long location info
    std::string msg = ec.message();
    size_t pos = msg.find(" [");
    if (pos != std::string::npos) {
        msg = msg.substr(0, pos);
    }
    return msg;
}

asio::awaitable<void> session(tcp::socket socket) {
    beast::error_code ec;
    try {
        beast::flat_buffer buffer;

        // First handle HTTP Upgrade
        http::request<http::string_body> req;
        co_await http::async_read(socket, buffer, req, asio::use_awaitable);

        // Verify it's a WebSocket upgrade request
        if (!websocket::is_upgrade(req)) {
            std::cerr << "Error: Not a WebSocket upgrade request\n";
            co_return;
        }

        websocket::stream<tcp::socket> ws(std::move(socket));
        
        // Set timeout and server decorator
        ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
        
        // Accept the websocket handshake
        co_await ws.async_accept(req, asio::use_awaitable);
        std::cout << "Info: WebSocket connection established\n";
        
        for (;;) {
            try {
                // Read a message
                co_await ws.async_read(buffer, asio::use_awaitable);
                
                std::string message = beast::buffers_to_string(buffer.data());
                std::cout << "Message: " << message << "\n";
                
                // Echo back with prefix
                std::string response = "Echo: " + message;
                ws.text(ws.got_text());
                co_await ws.async_write(asio::buffer(response), asio::use_awaitable);
                
                buffer.consume(buffer.size());
            }
            catch (boost::system::system_error& se) {
                std::cerr << "WebSocket: " << format_websocket_error(se.code()) << "\n";
                break;
            }
        }
        
    } catch (const boost::system::system_error& se) {
        std::cerr << "Session error: " << format_websocket_error(se.code()) << "\n";
    } catch (const std::exception& e) {
        std::cerr << "General error: " << e.what() << "\n";
    }
}

asio::awaitable<void> listener(std::string_view addres, std::string_view port)
{
    try {
        auto executor = co_await asio::this_coro::executor;
        const auto endpoint = tcp::endpoint(asio::ip::make_address(addres), std::stoi(std::string(port)));
        std::cout << "Info: Starting listener on " << addres << ":" << port << std::endl;

        beast::error_code ec;
        tcp::acceptor acceptor(executor);

        // Open the acceptor
        acceptor.open(endpoint.protocol(), ec);
        if (ec) {
            std::cerr << "Listener error: " << format_websocket_error(ec) << std::endl;
            co_return;
        }

        // Allow address reuse
        acceptor.set_option(asio::socket_base::reuse_address(true), ec);
        if (ec) {
            std::cerr << "Socket option error: " << format_websocket_error(ec) << std::endl;
            co_return;
        }
        // Bind to the server address
        acceptor.bind(endpoint, ec);

        if (ec) {
            std::cerr << "Bind error: " << ec.message() << std::endl;
            co_return;
        }

        acceptor.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) {
            std::cerr << "Listen error: " << ec.message() << std::endl;
            co_return;
        }

        for (;;) {
            auto socket = co_await acceptor.async_accept(asio::use_awaitable);
            asio::co_spawn(executor, session(std::move(socket)), asio::detached);
        }
    }
    catch (boost::system::system_error& se) {
        std::cerr << "WebSocket: " << format_websocket_error(se.code()) << "\n";
        co_return;
    }
    std::cout << "Listener stopped." << std::endl;
    co_return;
}

int main(int argc, char* argv[]) {
    asio::io_context ioc;

    // Add signal handler
    asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { ioc.stop(); });

    const auto addres = "127.0.0.1";
    const auto port = "8080";

    std::cout << "WebSocket server running on 127.0.0.1:8080" << std::endl;
    std::cout << "Connect with: ws://127.0.0.1:8080/" << std::endl;
    
    asio::co_spawn(ioc, listener(addres, port), asio::detached);
    ioc.run();
    return 0;
}