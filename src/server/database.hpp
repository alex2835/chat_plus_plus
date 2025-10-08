#pragma once
#include <string>
#include <map>
#include "datamodel.hpp"

class Database {
    std::map<std::string, ChatRoom> chatRooms;

public:
    Database() = default;
    ~Database() = default;

    void addMessage(const std::string& room, const ChatMessage& msg) {
        chatRooms[room].messages.push_back(msg);
    }

    std::vector<ChatMessage> getRoomMessages(const std::string& room) {
        return chatRooms[room].messages;
    }

    std::vector<std::string> getRoomNames() {
        std::vector<std::string> names;
        for (const auto& [name, _] : chatRooms)
            names.push_back(name);
        return names;
    }

};