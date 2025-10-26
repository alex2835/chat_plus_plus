#pragma once
#include "common/helpers.hpp"
#include "common/icontroller.hpp"
#include "common/request_datamodel.hpp"
#include "common/response_datamodel.hpp"
#include "database.hpp"
#include "server.hpp"

class OnInitSessionController : public IController
{
    Database& database_;
    Server& server_;

public:
    OnInitSessionController( Server& server, Database& database )
        : database_( database ),
          server_( server )
    {}
    ~OnInitSessionController() override = default;

    awaitable<void> call( const size_t sessionId, const json& /*msg*/ ) override
    {
        std::cout << "Info: OnInitSessionController called for session " << sessionId << "\n";

        auto roomsMessages = co_await database_.getRooms();
        InitSessionResponse response{ .roomsMessages = std::move( roomsMessages ) };
        auto message = makeMessage( ServerMessageType::InitSessionResponse, response );
        co_await server_.sendToSession( sessionId, message );
    }
};


class OnNewMessageController : public IController
{
    Database& database_;
    Server& server_;

public:
    OnNewMessageController( Server& server, Database& database )
        : database_( database ),
          server_( server )
    {}
    ~OnNewMessageController() override = default;

    awaitable<void> call( const size_t sessionId, const json& msg ) override
    {
        std::cout << "Info: OnNewMessageController called for session " << sessionId << "\n";

        auto request = msg.get<PostMessageRequest>();
        ChatMessage chatMessage{ .sender = request.user,
                                 .content = request.message,
                                 .timestamp = getTimestamp() };

        co_await database_.addMessage( request.room, std::move( chatMessage ) );

        NewMessage response{
            .room = request.room,
            .chatMessage = chatMessage
        };
        const auto message = makeMessage( ServerMessageType::NewMessage, response );
        co_await server_.broadcast( message );
    }
};


class OnNewRoomController : public IController
{
    Database& database_;
    Server& server_;

public:
    OnNewRoomController( Server& server, Database& database )
        : database_( database ),
          server_( server )
    {}
    ~OnNewRoomController() override = default;

    awaitable<void> call( const size_t sessionId, const json& msg ) override
    {
        std::cout << "Info: OnNewRoomController called for session " << sessionId << "\n";
        
        auto request = msg.get<PostRoomRequest>();
        co_await database_.addRoom( request.room );

        NewRoom response{ .room = request.room };
        auto message = makeMessage( ServerMessageType::NewRoom, response );
        co_await server_.broadcast( message );
    }
};