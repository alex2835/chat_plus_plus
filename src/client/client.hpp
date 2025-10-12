#pragma once
#include "pch.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include "database.hpp"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class ChatClient {
private:
    ClientDatabase& database_;

    std::atomic<bool> isConnected_{false};
    std::atomic<bool> isRunning_{true};

    asio::io_context ioContext_;
    websocket::stream<tcp::socket> websocket_;
    std::thread readThread_;
    
    // Client state
    std::string userName_;

    // UI components
    std::string statusText_ = "Disconnected";
    std::string serverAddress_ = "localhost";
    std::string serverPort_ = "8080";
    
public:
    ChatClient(ClientDatabase& database)
        : websocket_(ioContext_),
          database_(database)
    {}
    
    ~ChatClient() {
        disconnect();
        isRunning_ = false;
        if (readThread_.joinable())
            readThread_.join();
    }
    
    void connect() {
        if (isConnected_) return;
        ioContext_.restart();
        
        try {
            tcp::resolver resolver(ioContext_);
            auto endpoints = resolver.resolve(serverAddress_, serverPort_);
            
            // Connect the underlying TCP socket
            auto& socket = websocket_.next_layer();
            asio::connect(socket, endpoints);
            
            // Set these options for immediate message sending
            socket.set_option(tcp::no_delay(true));

            // Set timeout and server decorator
            websocket_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
            
            // Perform WebSocket handshake
            websocket_.handshake(serverAddress_, "/");
            
            isConnected_ = true;
            statusText_ = "Connected to " + serverAddress_ + ":" + serverPort_;
            
            // Send userName as first message
            if (!userName_.empty()) {
                std::string joinMessage = "/join " + userName_;
                sendMessage(joinMessage);
            }
            
            // Start reading messages
            startReadLoop();
            
        } catch (std::exception& e) {
            statusText_ = "Connection failed: " + std::string(e.what());
            isConnected_ = false;
        }
    }

    void disconnect() {
        if (!isConnected_) return;
        isConnected_ = false;
        try {
            if (websocket_.is_open()) {
                websocket_.close(websocket::close_code::normal);
            }
        } catch (...) {}

        ioContext_.stop();
        if (readThread_.joinable())
            readThread_.join();

        statusText_ = "Disconnected";
    }
    
    void sendMessage(const std::string& message) {
        if (!isConnected_ || message.empty())
            return;
        
        try {
            websocket_.text(true);
            websocket_.async_write(asio::buffer(message),
            [](boost::beast::error_code ec, std::size_t bytesWritten) {
                if (!ec) {
                    std::cout << "Wrote " << bytesWritten << " bytes\n";
                }
            });
        } catch (std::exception& e) {
            statusText_ = "[Error] Failed to send message: " + std::string(e.what());
            disconnect();
        }
    }

    void setUserName(const std::string& name) {
        userName_ = name;
    }
    
    void setServer(const std::string& address, const std::string& port) {
        serverAddress_ = address;
        serverPort_ = port;
    }
    
    std::string getStatus() const {
        return statusText_;
    }
    
    bool isConnected() const {
        return isConnected_;
    }
    
private:
    void startReadLoop() {
        readThread_ = std::thread([this]() {
            beast::error_code ec;
            while (isConnected_ && isRunning_) {
                try {
                    beast::flat_buffer buffer;
                    websocket_.read(buffer);

                    auto message = beast::buffers_to_string(buffer.data());
                    auto messageJson = nlohmann::json::parse(std::move(message));


                    buffer.consume(buffer.size());
                } 
                catch (std::exception& e)
                {
                    if (isConnected_) {
                        statusText_ = "[Error] Read failed: " + std::string(e.what());
                        disconnect();
                    }
                    break;
                }
            }
        });
    }
    
    std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto timePoint = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&timePoint), "%H:%M:%S");
        return ss.str();
    }
};


