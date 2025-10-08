#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using namespace asio::experimental::awaitable_operators;

class HttpServer {
public:
    HttpServer(asio::io_context& ioc, unsigned short port)
        : ioc_(ioc), acceptor_(ioc, {asio::ip::tcp::v4(), port}) {}

    asio::awaitable<void> run() {
        while (true) {
            auto socket = co_await acceptor_.async_accept(asio::use_awaitable);
            asio::co_spawn(ioc_, handleSession(std::move(socket)), asio::detached);
        }
    }

private:
    asio::awaitable<void> handleSession(asio::ip::tcp::socket socket) {
        try {
            beast::flat_buffer buffer;
            
            while (true) {
                http::request<http::string_body> req;
                co_await http::async_read(socket, buffer, req, asio::use_awaitable);
                
                auto res = handleRequest(req);
                co_await http::async_write(socket, res, asio::use_awaitable);
                
                if (!req.keep_alive()) {
                    break;
                }
            }
        }
        catch (const std::exception& e) {
            std::cout << "Session error: " << e.what() << std::endl;
        }
        
        socket.shutdown(asio::ip::tcp::socket::shutdown_send);
    }

    http::response<http::string_body> handleRequest(const http::request<http::string_body>& req) {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Beast-Coroutines");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());

        // Simple routing
        if (req.target() == "/") {
            res.body() = R"x({"message": "Hello from Beast Coroutines!", "method": ")" + 
                        std::string(req.method_string()) + R"("})x";
        }
        else if (req.target() == "/health") {
            res.body() = R"({"status": "healthy", "server": "coroutines"})";
        }
        else if (req.target() == "/echo" && req.method() == http::verb::post) {
            res.body() = R"({"echo": ")" + req.body() + R"("})";
        }
        else {
            res.result(http::status::not_found);
            res.body() = R"({"error": "Route not found"})";
        }

        res.prepare_payload();
        return res;
    }

    asio::io_context& ioc_;
    asio::ip::tcp::acceptor acceptor_;
};

asio::awaitable<void> startServer() {
    auto executor = co_await asio::this_coro::executor;
    auto& ioc = static_cast<asio::io_context&>(executor.context());
    
    HttpServer server(ioc, 8080);
    std::cout << "HTTP server with coroutines running on http://localhost:8080" << std::endl;
    
    co_await server.run();
}

int main() {
    asio::io_context ioc;
    
    asio::co_spawn(ioc, startServer(), asio::detached);
    ioc.run();
    
    return 0;
}
