#include "pch.hpp"
#include "database.hpp"
#include "client.hpp"
#include "client_ui.hpp"

int main()
{
    ClientDatabase database;
    ChatClient client(database);
    ChatClientUI ui(client, database);
    ui.run();
    return 0;
}
