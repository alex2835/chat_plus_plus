#pragma once
#include "pch.hpp"
#include <string>
#include <vector>

enum class ResponseType
{
    Rooms,
    RoomMessages,

    NewRoom,
    NewMessage
};

struct RoomsResponse
{
    std::vector<std::string> roomNames;
};
NLOJHMANN_DEFINE_TYPE_INTRUSIVE(RoomsResponse, roomNames)


struct RoomMessagesResponse
{
    std::string roomName;
    std::vector<std::string> messages;
};
NLOJHMANN_DEFINE_TYPE_INTRUSIVE(RoomsResponse, roomNames)


struct NewRoomResponse
{
    std::string roomName;
};
NLOJHMANN_DEFINE_TYPE_INTRUSIVE(NewRoomResponse, roomName)


struct NewMessageResponse
{
    std::string roomName;
    std::string sender;
    std::string content;
};
NLOJHMANN_DEFINE_TYPE_INTRUSIVE(NewMessageResponse, roomName, sender, content)