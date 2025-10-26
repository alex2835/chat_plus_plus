#pragma once
#include "common/datamodel.hpp"

// In memmory database for example purposes
class Database
{
    asio::io_context& ioContext_;
    asio::strand<asio::io_context::executor_type> strand_;
    std::unordered_map<std::string, ChatRoom> chatRooms_;

public:
    Database( asio::io_context& ioContext ) 
        : ioContext_( ioContext ),
          strand_( asio::make_strand( ioContext ) )
    {}
    ~Database() = default;

    awaitable<void> addMessage( const std::string& room, const ChatMessage& msg )
    {
        co_await asio::post( asio::bind_executor( strand_, asio::use_awaitable ) );
        chatRooms_[room].messages.push_back( msg );
    }

    awaitable<std::vector<ChatMessage>> getRoomMessages( const std::string& room ) const
    {
        co_await asio::post( asio::bind_executor( strand_, asio::use_awaitable ) );
        auto it = chatRooms_.find( room );
        if ( it == chatRooms_.end() )
            co_return std::vector<ChatMessage>();
        co_return it->second.messages;
    }

    awaitable<std::vector<std::string>> getRoomNames() const
    {
        co_await asio::post( asio::bind_executor( strand_, asio::use_awaitable ) );
        std::vector<std::string> names;
        for ( const auto& [name, _] : chatRooms_ )
            names.push_back( name );
        co_return names;
    }

    awaitable<std::vector<ChatRoom>> getRoom() const
    {
        co_await asio::post( asio::bind_executor( strand_, asio::use_awaitable ) );
        std::vector<ChatRoom> rooms;
        for ( const auto& [_, room] : chatRooms_ )
            rooms.push_back( room );
        co_return rooms;
    }
};