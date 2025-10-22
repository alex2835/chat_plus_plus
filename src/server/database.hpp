#pragma once
#include "datamodel.hpp"

class Database
{
    std::unordered_map<std::string, ChatRoom> chatRooms_;

public:
    Database() = default;
    ~Database() = default;

    void addMessage( const std::string& room, const ChatMessage& msg )
    {
        chatRooms_[room].messages.push_back( msg );
    }

    std::vector<ChatMessage> getRoomMessages( const std::string& room )
    {
        return chatRooms_[room].messages;
    }

    std::vector<std::string> getRoomNames()
    {
        std::vector<std::string> names;
        for ( const auto& [name, _] : chatRooms_ )
            names.push_back( name );
        return names;
    }
};