#pragma once
#include <string>
#include <vector>

struct ChatMessage {
    std::string sender;
    std::string content;
    std::string timestamp;
};

struct ChatRoom {
    std::string name;
    std::vector<ChatMessage> messages;
};
