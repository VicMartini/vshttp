#include "server.hpp"
#include <iostream>

#include <csignal>
#include <getopt.h>

Server *server_ptr = nullptr;

void handleShutdown(int signal)

{
    if (signal == SIGINT && server_ptr)
    {
        std::cout << "Shutting down server..." << std::endl;
        server_ptr->shutdown();
    }
}

void printAsciiArt()
{
    std::cout << "\033[32m"
              <<
        R"(
░▒▓█▓▒░░▒▓█▓▒░░▒▓███████▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓████████▓▒░▒▓████████▓▒░▒▓███████▓▒░  
░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░  ░▒▓█▓▒░      ░▒▓█▓▒░   ░▒▓█▓▒░░▒▓█▓▒░ 
 ░▒▓█▓▒▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░  ░▒▓█▓▒░      ░▒▓█▓▒░   ░▒▓█▓▒░░▒▓█▓▒░ 
 ░▒▓█▓▒▒▓█▓▒░ ░▒▓██████▓▒░░▒▓████████▓▒░  ░▒▓█▓▒░      ░▒▓█▓▒░   ░▒▓███████▓▒░  
  ░▒▓█▓▓█▓▒░        ░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░  ░▒▓█▓▒░      ░▒▓█▓▒░   ░▒▓█▓▒░        
  ░▒▓█▓▓█▓▒░        ░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░  ░▒▓█▓▒░      ░▒▓█▓▒░   ░▒▓█▓▒░        
   ░▒▓██▓▒░  ░▒▓███████▓▒░░▒▓█▓▒░░▒▓█▓▒░  ░▒▓█▓▒░      ░▒▓█▓▒░   ░▒▓█▓▒░                                               
    )"
              << "\033[0m" << std::endl;
}

void printServerInfo(int port, const std::string &files_dir)
{
    std::cout << "Listening on port: " << port << "\n"
              << "Serving files from: " << files_dir << std::endl;
}

int main(int argc, char *argv[])
{
    printAsciiArt();

    int port = 8080;
    size_t num_workers = 16;
    std::string files_dir = ".";

    const struct option long_options[] = {{"port", required_argument,

                                           nullptr, 'p'},
                                          {"workers", required_argument, nullptr, 'w'},
                                          {"files_dir", required_argument, nullptr, 'f'},
                                          {nullptr, 0, nullptr, 0}};

    int opt;
    while ((opt = getopt_long(argc, argv, "p:w:f:", long_options, nullptr)) != -1)
    {
        switch (opt)
        {
        case 'p':
            port = std::stoi(optarg);
            break;
        case 'w':
            num_workers = std::stoul(optarg);
            break;
        case 'f':
            files_dir = optarg;
            break;
        default:
            std::cerr << "Usage: " << argv[0] << " [--port <port>] [--workers <num_workers>] [--files_dir <directory>]"
                      << std::endl;
            return 1;
        }
    }
    // TODO: Check a better mechanism to pass the files_dir to the workers
    setenv("FILES_DIR", files_dir.c_str(), 1);

    Server server(port, num_workers);
    server_ptr = &server;

    std::signal(SIGINT, handleShutdown);

    server.start();
    printServerInfo(port, files_dir);
    server.wait();
}