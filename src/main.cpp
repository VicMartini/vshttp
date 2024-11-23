#include "server.hpp"
#include <iostream>

#include <csignal>

Server *server_ptr = nullptr;

void handleShutdown(int signal)
{
    if (signal == SIGTSTP && server_ptr)
    {
        server_ptr->shutdown();
    }
}

int main()
{
    std::cout << "vshttp" << std::endl;

    Server server(8080, 16);
    server_ptr = &server;

    std::signal(SIGTSTP, handleShutdown);

    server.start();
    std::cout << "Listening on port 8080" << std::endl;
    server.wait();
}