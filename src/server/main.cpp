#include "pch.hpp"
#include "database.hpp"
#include "common/request_datamodel.hpp"
#include "server_controllers.hpp"
#include "server.hpp"

int main( int argc, char* argv[] )
{
    Server server( "127.0.0.1", 8080 );

    Database database( server.getIOContext() );
    server.addController( ClientMessageType::InitSession, OnInitSessionController( server, database ) );
    server.addController( ClientMessageType::PostNewRoom, OnNewRoomController( server, database ) );
    server.addController( ClientMessageType::PostMessage, OnNewMessageController( server, database ) );

    server.run();
    return 0;
}
