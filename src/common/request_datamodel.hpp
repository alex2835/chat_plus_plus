#pragma once
#include "pch.hpp"
#include <string>
#include <vector>

enum class RequestType {
    GetRooms,
    GetRoomMessages,
    
    PostRoom,
    PostMessage
};

struct GetRoomMessagesRequest {
    std::string roomName;
};
NLOHMANN_DEFINE_TYPE_INTRUSIVE(GetRoomMessagesRequest, roomName)


struct PostRoomRequest {
    std::string roomName;
};
NLOHMANN_DEFINE_TYPE_INTRUSIVE(PostRoomRequest, roomName)


struct PostMessageRequest {
    std::string roomName;
    std::string sender;
    std::string content;
};
NLOHMANN_DEFINE_TYPE_INTRUSIVE(PostMessageRequest, roomName, sender, content)