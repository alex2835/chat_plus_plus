#pragma once
#include "common/datamodel.hpp"

class ClientData
{
    std::string userName_;
    std::unordered_map<std::string, ChatRoom> chats_;
    std::mutex messagesMutex_;

public:
    ClientData() = default;
    ~ClientData() = default;

    void setUserName( std::string name )
    {
        userName_ = std::move( name );
    }

    const std::string& getUserName()
    {
        return userName_;
    }

    void addMessage( const std::string& room, 
                     const ChatMessage& msg )
    {
        std::scoped_lock lock( messagesMutex_ );
        chats_[room].messages.push_back( msg );
    }

    void addMessages( const std::string& room, 
                      const std::vector<ChatMessage>& msgs )
    {
        std::scoped_lock lock( messagesMutex_ );
        for ( const auto& msg : msgs )
            chats_[room].messages.push_back( msg );
    }

    std::vector<std::string> getRoomNames()
    {
        std::scoped_lock lock( messagesMutex_ );
        std::vector<std::string> names;
        for ( const auto& [name, _] : chats_ )
            names.push_back( name );
        return names;
    }

    std::vector<ChatMessage> getRoomMessages( const std::string& room )
    {
        std::scoped_lock lock( messagesMutex_ );
        auto it = chats_.find( room );
        if ( it == chats_.end() )
            return {};
        return it->second.messages;
    }
};