#pragma once
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

struct PostRoomRequest {
    std::string roomName;
};

struct PostMessageRequest {
    std::string roomName;
    std::string sender;
    std::string content;
};