#pragma once
#include "datamodel.hpp"

enum class ServerMessageType
{
    InitSessionResponse,

    NewRoom,
    NewMessage
};

struct InitSessionResponse
{
    std::vector<ChatRoom> roomsMessages;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE( InitSessionResponse, roomsMessages )
};

struct NewRoom
{
    std::string room;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE( NewRoom, room )
};

struct NewMessage
{
    std::string room;
    ChatMessage chatMessage;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE( NewMessage, room, chatMessage )
};