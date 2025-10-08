#pragma once
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

struct RoomMessagesResponse
{
    std::string roomName;
    std::vector<std::string> messages;
};

struct NewRoomResponse
{
    std::string roomName;
};

struct NewMessageResponse
{
    std::string roomName;
    std::string sender;
    std::string content;
};