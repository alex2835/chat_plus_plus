#pragma once
#include "common/datamodel.hpp"

class ClientDatabase
{
    std::unordered_map<std::string, ChatMessage> Chats_;
    std::mutex messagesMutex_;

public:
    ClientDatabase() = default;
    ~ClientDatabase() = default;
};