#pragma once
#include "datamodel.hpp"

enum class ServerMessageType
{
    RoomsMessagesResponse,

    NewRoom,
    NewMessage
};

struct RoomsMessagesResponse
{
    std::vector<ChatRoom> roomsMessages;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE( RoomsMessagesResponse, roomsMessages )
};

struct NewRoom
{
    std::string room;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE( NewRoom, room )
};

struct NewMessage
{
    std::string room;
    ChatMessage message;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE( NewMessage, room, message )
};