#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;


//------------------------------------------------------------------------------
// WebSocket Client
//------------------------------------------------------------------------------
class WebSocketClient {
private:
    net::io_context& ioc_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string port_;
    std::string target_;
    
public:
    WebSocketClient(net::io_context& ioc, const std::string& host, 
                   const std::string& port, const std::string& target)
        : ioc_(ioc), 
          ws_(net::make_strand(ioc)), 
          host_(host), 
          port_(port),
          target_(target) {}
    
    void connect() {
        // Look up the domain name
        tcp::resolver resolver(ioc_);
        auto const results = resolver.resolve(host_, port_);
        
        // Set a timeout on the operation
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));
        
        // Make the connection on the IP address we get from a lookup
        auto ep = beast::get_lowest_layer(ws_).connect(results);
        
        // Update the host string for the HTTP handshake
        std::string host_port = host_ + ':' + std::to_string(ep.port());
        
        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(ws_).expires_never();
        
        // Set suggested timeout settings for the websocket
        ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
        
        // Set a decorator to change the User-Agent of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(http::field::user_agent, "Beast WebSocket Client");
            }));
        
        // Perform the websocket handshake
        ws_.handshake(host_port, target_);
        
        std::cout << "Connected to WebSocket server" << std::endl;
    }
    
    void send_message(const std::string& message) {
        ws_.write(net::buffer(message));
        std::cout << "Sent: " << message << std::endl;
    }
    
    std::string read_message() {
        buffer_.clear();
        ws_.read(buffer_);
        std::string message = beast::buffers_to_string(buffer_.data());
        std::cout << "Received: " << message << std::endl;
        return message;
    }
    
    void close() {
        ws_.close(websocket::close_code::normal);
        std::cout << "Connection closed" << std::endl;
    }
};

//------------------------------------------------------------------------------
// Async WebSocket Client (C++20 Coroutines)
//------------------------------------------------------------------------------
#ifdef __cpp_impl_coroutine
net::awaitable<void> async_websocket_client(std::string host, std::string port, std::string target) {
    auto executor = co_await net::this_coro::executor;
    websocket::stream<beast::tcp_stream> ws(executor);
    
    try {
        // Resolve and connect
        tcp::resolver resolver(executor);
        auto results = co_await resolver.async_resolve(host, port, net::use_awaitable);
        auto ep = co_await beast::get_lowest_layer(ws).async_connect(results, net::use_awaitable);
        
        // WebSocket handshake
        std::string host_port = host + ':' + std::to_string(ep.port());
        co_await ws.async_handshake(host_port, target, net::use_awaitable);
        
        std::cout << "Async client connected" << std::endl;
        
        // Send some messages
        co_await ws.async_write(net::buffer("Hello from async client!"), net::use_awaitable);
        
        // Read response
        beast::flat_buffer buffer;
        co_await ws.async_read(buffer, net::use_awaitable);
        std::cout << "Async received: " << beast::buffers_to_string(buffer.data()) << std::endl;
        
        // Close
        co_await ws.async_close(websocket::close_code::normal, net::use_awaitable);
        
    } catch (std::exception const& e) {
        std::cerr << "Async client error: " << e.what() << std::endl;
    }
}
#endif

//------------------------------------------------------------------------------
// Example Usage
//------------------------------------------------------------------------------

void run_client() {
    net::io_context ioc;
    
    try {
        WebSocketClient client(ioc, "127.0.0.1", "8080", "/");
        client.connect();
        
        // Send some messages
        client.send_message("Hello, WebSocket!");
        client.read_message();
        
        client.send_message("How are you?");
        client.read_message();
        
        client.send_message("Goodbye!");
        client.read_message();
        
        client.close();
        
    } catch (std::exception const& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    
    run_client();
    return 0;
}