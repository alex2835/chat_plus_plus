#pragma once
#include <string>
#include <vector>

struct ChatMessage {
    uint64_t id;
    std::string sender;
    std::string content;
    std::string timestamp;
};

struct ChatRoom {
    uint64_t messageIdCounter = 0;
    std::string name;
    std::vector<ChatMessage> messages;
};
