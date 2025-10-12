#pragma once
#include "pch.hpp"
#include "common/datamodel.hpp"

class ClientDatabase {
    std::unordered_map<std::string, ChatMessage> messages_;
    std::mutex messagesMutex_;

public:
    ClientDatabase() = default;
    ~ClientDatabase() = default;
};