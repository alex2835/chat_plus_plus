#include "pch.hpp"
#include "client.hpp"
#include "client_data.hpp"
#include "client_ui.hpp"

int main()
{
    ClientData clientData;
    ChatClient client( clientData );
    ChatClientUI ui( clientData, client );
    ui.run();
    return 0;
}
