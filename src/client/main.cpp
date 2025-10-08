#include "client.hpp"
#include "client_ui.hpp"

int main()
{
    ChatClient client;
    ChatClientUI ui(client);
    ui.run();
    return 0;
}
