#include "server.hpp"

#include <cerrno>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include <thread>

// Public methods
Server::Server(int port, size_t num_workers) : port_(port), num_workers_(num_workers)
{
    // Create socket
    if ((socket_fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        throw std::runtime_error("Failed to creat socket");
    }
    addr_ = createSockAddr();
    bindSocket();
    startListening();
}

void Server::start()
{
    // Start accepting clients in a new thread
    acceptor_thread_ = std::thread(&Server::acceptClients, this);

    // Start worker threads
    workers_.reserve(num_workers_);
    for (size_t i = 0; i < num_workers_; ++i)
    {
        Worker *worker = new Worker(i);
        worker->start(client_queue_, shutdown_);
        workers_.emplace_back(std::move(worker));
    }
}

Server::~Server()
{
    if (socket_fd_ >= 0)
    {
        close(socket_fd_);
    }
};

void Server::wait()
{
    if (acceptor_thread_.joinable())
    {
        acceptor_thread_.join();
    }

    // Wait for all workers
    for (auto &worker : workers_)
    {
        worker->wait();
    }
}

void Server::shutdown()
{
    shutdown_ = true;

    // Close the server socket to stop accepting new connections
    if (socket_fd_ >= 0)
    {
        close(socket_fd_);
        socket_fd_ = -1;
    }

    // Wake up all workers
    client_queue_.push(-1);

    // Wait for the acceptor thread to finish
    if (acceptor_thread_.joinable())
    {
        acceptor_thread_.join();
    }

    // Wait for all workers to finish
    for (auto &worker : workers_)
    {
        worker->wait();
        delete worker;
    }
    workers_.clear();
}

// Private methods
sockaddr_in Server::createSockAddr() const
{
    /**
     *Create IPv4 socket address structure
     */
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    return addr;
}

void Server::bindSocket()
{
    if (bind(socket_fd_, (struct sockaddr *)&addr_, sizeof(addr_)) < 0)
    {
        throw std::system_error(errno, std::system_category(), "Failed to bind socket");
    }
}

void Server::acceptClient()
{
    int client_fd;
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Accept connection
    if ((client_fd = accept(socket_fd_, (struct sockaddr *)&client_addr, &client_len)) < 0)
    {
        throw std::system_error(errno, std::system_category(), "Failed to accept connection");
    }
    // Add to queue so a worker can pick it up, no need to lock, see threadsafe_queue.hpp
    client_queue_.push(client_fd);

    // Log the new connection
    std::string client_ip = inet_ntoa(client_addr.sin_addr);
    uint16_t client_port = ntohs(client_addr.sin_port);

    std::cout << "New connection from " << client_ip
              << ":" << client_port << std::endl;
    std::cout << "Queue size: " << client_queue_.size() << std::endl;
}

void Server::acceptClients()
{
    std::cout << "Accepting clients" << std::endl;
    while (!shutdown_)
    {
        acceptClient();
    }
}

void Server::startListening()
{
    if (listen(socket_fd_, 10) < 0)
    {
        throw std::system_error(errno, std::system_category(),
                                "Failed when calling listen");
    }
}