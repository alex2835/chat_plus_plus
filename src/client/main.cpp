#include "client.hpp"
#include "client_database.hpp"
#include "client_ui.hpp"
#include "pch.hpp"

int main()
{
    ClientDatabase database;
    ChatClient client( database );
    ChatClientUI ui( client, database );
    ui.run();
    return 0;
}
