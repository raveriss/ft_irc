#include <iostream>
#include <cstdlib>
#include "server.hpp"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: ./micro_ircserv <port> <password>" << std::endl;
        return 1;
    }

    int port = atoi(argv[1]);
    std::string password = argv[2];

    if (port <= 0 || port > 65535) {
        std::cerr << "Error: Invalid port number." << std::endl;
        return 1;
    }

    try {
        Server server(port, password);
        server.run();
    } catch (const std::exception &e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}


