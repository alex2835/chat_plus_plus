#pragma once

enum class ClientMessageType
{
    InitSession,
    PostUserName,
    PostNewRoom,
    PostMessage
};

struct PostRoomRequest
{
    std::string room;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE( PostRoomRequest, room )
};

struct PostMessageRequest
{
    std::string user;
    std::string room;
    std::string message;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE( PostMessageRequest, user, room, message )
};