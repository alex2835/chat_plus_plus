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
    NLOJHMANN_DEFINE_TYPE_INTRUSIVE( RoomsMessagesResponse, roomsMessages )
};

struct NewRoom
{
    std::string room;
    NLOJHMANN_DEFINE_TYPE_INTRUSIVE( NewRoomResponse, room )
};

struct NewMessage
{
    std::string room;
    ChatMessage message;
    NLOJHMANN_DEFINE_TYPE_INTRUSIVE( NewMessageResponse, room, message )
};