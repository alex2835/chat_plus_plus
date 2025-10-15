#include "pch.hpp"
#include "server.hpp"

int main(int argc, char* argv[]) {
    Server server("127.0.0.1", 8080);
    server.run();
    return 0;
}
