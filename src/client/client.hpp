#pragma once
#include "pch.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class ChatClient {
private:
    asio::io_context io_context;
    websocket::stream<tcp::socket> ws;
    std::thread read_thread;
    
    std::atomic<bool> connected{false};
    std::atomic<bool> running{true};

    std::vector<std::string> messages;
    std::mutex messages_mutex;
    std::string username;

    // UI components
    std::string status_text = "Disconnected";
    std::string server_address = "localhost";
    std::string server_port = "8080";
    
public:
    ChatClient() : ws(io_context) {}
    
    ~ChatClient() {
        disconnect();
        running = false;
        if (read_thread.joinable()) read_thread.join();
    }
    
    void connect() {
        if (connected) return;
        
        // Ensure clean state before connecting
        clean_connection();
        
        try {
            // Reset io_context and create a new websocket stream
            io_context.restart();
            // ws = websocket::stream<tcp::socket>(io_context);
            
            tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(server_address, server_port);
            
            // Connect the underlying TCP socket
            auto& socket = ws.next_layer();
            asio::connect(socket, endpoints);
            
            // Set these options for immediate message sending
            socket.set_option(tcp::no_delay(true));

            // Set timeout and server decorator
            ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
            
            // Perform WebSocket handshake
            ws.handshake(server_address, "/");
            
            connected = true;
            status_text = "Connected to " + server_address + ":" + server_port;
            
            // Send username as first message
            if (!username.empty()) {
                std::string join_msg = "/join " + username;
                send_message(join_msg);
            }
            
            // Start reading messages
            start_read();
            
        } catch (std::exception& e) {
            status_text = "Connection failed: " + std::string(e.what());
            connected = false;
        }
    }
    
    void clean_connection() {
        // Stop the read thread first
        running = false;
        if (read_thread.joinable()) {
            read_thread.join();
        }
        
        try {
            if (ws.is_open()) {
                beast::error_code ec;
                ws.close(websocket::close_code::normal, ec);
                // Ignore any errors during close
            }
        } catch (...) {}
        
        connected = false;
    }

    void disconnect() {
        if (!connected) return;
        
        try {
            if (ws.is_open()) {
                // Close the WebSocket connection gracefully
                ws.close(websocket::close_code::normal);
            }
        } catch (...) {}
        
        clean_connection();
        status_text = "Disconnected";
        add_message("[System] Disconnected from server");
    }
    
    void send_message(const std::string& msg) {
        if (!connected || msg.empty()) return;
        
        try {
            // Set text mode for the message
            ws.text(true);
            ws.write(asio::buffer(msg));
            
            // Add to local display with timestamp
            std::string display_msg = "[" + get_timestamp() + "] " + username + ": " + msg;
            add_message(display_msg);
        } catch (std::exception& e) {
            add_message("[Error] Failed to send message: " + std::string(e.what()));
            disconnect();
        }
    }
    
    void add_message(const std::string& msg) {
        std::lock_guard<std::mutex> lock(messages_mutex);
        messages.push_back(msg);
        
        // Keep message history limited
        if (messages.size() > 100) {
            messages.erase(messages.begin());
        }
    }
    
    std::vector<std::string> get_messages() {
        std::lock_guard<std::mutex> lock(messages_mutex);
        return messages;
    }
    
    void set_username(const std::string& name) {
        username = name;
    }
    
    void set_server(const std::string& addr, const std::string& port) {
        server_address = addr;
        server_port = port;
    }
    
    std::string get_status() const {
        return status_text;
    }
    
    bool is_connected() const {
        return connected;
    }
    
private:
    void start_read() {
        read_thread = std::thread([this]() {
            beast::error_code ec;
            while (connected && running) {
                try {
                    beast::flat_buffer buffer;
                    ws.read(buffer, ec);
                    
                    if (ec) {
                        if (connected) {
                            add_message("[Error] Read failed: " + ec.message());
                            disconnect();
                        }
                        break;
                    }
                    
                    // Convert message to string
                    std::string message = beast::buffers_to_string(buffer.data());
                    
                    if (!message.empty()) {
                        // Add timestamp to received messages
                        std::string display_msg = "[" + get_timestamp() + "] " + message;
                        add_message(display_msg);
                    }
                } catch (std::exception& e) {
                    if (connected) {
                        add_message("[Error] Read failed: " + std::string(e.what()));
                        disconnect();
                    }
                    break;
                }
            }
        });
    }
    
    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        return ss.str();
    }
};


