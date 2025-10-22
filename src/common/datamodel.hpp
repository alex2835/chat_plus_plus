#pragma once

struct ChatMessage
{
    std::string sender;
    std::string content;
    std::string timestamp;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE( ChatMessage, sender, content, timestamp )
};

struct ChatRoom
{
    std::string name;
    std::vector<ChatMessage> messages;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE( ChatRoom, name, messages )
};
