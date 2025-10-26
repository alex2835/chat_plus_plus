#pragma once
#include "client_data.hpp"
#include "common/message.hpp"
#include "common/response_datamodel.hpp"
#include "common/request_datamodel.hpp"
#include <queue>
#include <thread>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class ChatClient
{
private:
    ClientData& clientData_;

    std::atomic<bool> isConnected_{ false };
    std::atomic<bool> isRunning_{ true };

    asio::io_context ioContext_;
    websocket::stream<tcp::socket> websocket_;

    std::thread readThread_;
    std::thread writeThread_;
    std::queue<std::string> writeQueue_;
    std::mutex writeQueueMutex_;
    std::condition_variable writeQueueCondVar_;

    // UI components
    std::string connectionStatus_ = "Disconnected";
    std::string connectionErrorStatus_;
    std::string serverAddress_ = "localhost";
    std::string serverPort_ = "8080";

public:
    ChatClient( ClientData& clientData ) 
    : websocket_( ioContext_ ), 
      clientData_( clientData )
    {
        websocket_.text( true );
    }

    ~ChatClient()
    {
        disconnect();
        isRunning_ = false;
        writeQueueCondVar_.notify_all();
        if ( writeThread_.joinable() )
            writeThread_.join();
        if ( readThread_.joinable() )
            readThread_.join();
    }

    void connect()
    {
        if ( isConnected_ )
            return;

        try
        {
            tcp::resolver resolver( ioContext_ );
            auto endpoints = resolver.resolve( serverAddress_, serverPort_ );

            // Connect the underlying TCP socket
            auto& socket = websocket_.next_layer();
            asio::connect( socket, endpoints );

            // Set these options for immediate message sending
            socket.set_option( tcp::no_delay( true ) );

            // Set timeout and server decorator
            websocket_.set_option( websocket::stream_base::timeout::suggested( beast::role_type::client ) );

            // Perform WebSocket handshake
            websocket_.handshake( serverAddress_, "/" );

            isConnected_ = true;
            connectionStatus_ = "Connected to " + serverAddress_ + ":" + serverPort_;

            // Start read and write threads
            startReadLoop();
            startWriteLoop();

            // Request initial chat rooms and messages
            sendMessage( makeMessage( ClientMessageType::InitSession, json() ) );
        }
        catch ( std::exception& e )
        {
            connectionStatus_ = "Connection failed: " + std::string( e.what() );
            isConnected_ = false;
        }
    }

    void disconnect()
    {
        if ( not isConnected_ )
            return;
        isConnected_ = false;
        connectionStatus_ = "Disconnected";

        writeQueueCondVar_.notify_all();  // Wake up write thread

        try
        {
            if ( websocket_.is_open() )
                websocket_.close( websocket::close_code::normal );
        }
        catch ( ... )
        {
        }

        auto currentThreadId = std::this_thread::get_id();

        // Join write thread if not calling from it
        if ( writeThread_.joinable() and
             currentThreadId != writeThread_.get_id() )
            writeThread_.join();

        // Join read thread if not calling from it
        if ( readThread_.joinable() and
             currentThreadId != readThread_.get_id() )
            readThread_.join();
    }

    void setServer( const std::string& address, const std::string& port )
    {
        serverAddress_ = address;
        serverPort_ = port;
    }

    std::string getStatus() const
    {
        return connectionStatus_;
    }

    bool isConnected() const
    {
        return isConnected_;
    }

    void sendChatMessage( const std::string& room,
                          const std::string& content )
    {
        PostMessageRequest req{ 
            .user = clientData_.getUserName(),
            .room = room,
            .message = content,
        };
        sendMessage( makeMessage( ClientMessageType::PostMessage, req ) );
    }

    void sendChatRoom( const std::string& room )
    {
        PostRoomRequest req{ .room = room };
        sendMessage( makeMessage( ClientMessageType::PostNewRoom, req ) );
    }

private:
    void sendMessage( const std::string& message )
    {
        if ( not isConnected_ or message.empty() )
            return;

        {
            std::scoped_lock lock( writeQueueMutex_ );
            writeQueue_.push( message );
        }
        writeQueueCondVar_.notify_one();  // Wake up write thread
    }

    void startReadLoop()
    {
        readThread_ = std::thread(
            [this]()
            {
                while ( isConnected_ and isRunning_ )
                {
                    try
                    {
                        beast::flat_buffer buffer;
                        websocket_.read( buffer );
                        auto message = beast::buffers_to_string( buffer.data() );
                        json messageJson = nlohmann::json::parse( std::move( message ) );
                        
                        auto typeStr = messageJson.at( "metadata" ).at( "type" ).get<std::string>();
                        auto type = magic_enum::enum_cast<ServerMessageType>( typeStr );
                        json dataJson = messageJson["data"];
                        
                        switch ( type.value() )
                        {
                            case ServerMessageType::InitSessionResponse:
                            {
                                auto response = dataJson.get<InitSessionResponse>();
                                for ( const auto& room : response.roomsMessages )
                                    clientData_.addMessages( room.name, room.messages );
                                break;
                            }
                            case ServerMessageType::NewRoom:
                            {
                                auto response = dataJson.get<NewRoom>();
                                clientData_.addRoom( response.room );
                                break;
                            }
                            case ServerMessageType::NewMessage:
                            {
                                auto response = dataJson.get<NewMessage>();
                                clientData_.addMessage( response.room, response.chatMessage );
                                break;
                            }
                        }
                        buffer.consume( buffer.size() );
                    }
                    catch ( std::exception& e )
                    {
                        if ( isConnected_ )
                        {
                            connectionErrorStatus_ = std::format( "[Error] Read failed: {}", e.what() );
                            disconnect();
                        }
                        break;
                    }
                }
            } );
    }

    void startWriteLoop()
    {
        writeThread_ = std::thread(
            [this]()
            {
                while ( isRunning_ and isConnected_ )
                {
                    std::unique_lock<std::mutex> lock( writeQueueMutex_ );

                    // Wait for messages to send or shutdown signal
                    writeQueueCondVar_.wait( lock, 
                        [this]() { return not writeQueue_.empty() or
                                          not isRunning_ or 
                                          not isConnected_; } );

                    if ( not isRunning_ or not isConnected_ )
                        break;

                    if ( not writeQueue_.empty() and isConnected_ )
                    {
                        auto message = std::move( writeQueue_.front() );
                        writeQueue_.pop();
                        lock.unlock();

                        try
                        {
                            websocket_.write( asio::buffer( message ) );
                        }
                        catch ( std::exception& e )
                        {
                            if ( isConnected_ )
                            {
                                connectionErrorStatus_ = std::format( "[Error] Write failed: {}", e.what() );
                                disconnect();
                            }
                            break;
                        }
                    }
                }
            } );
    }
};
