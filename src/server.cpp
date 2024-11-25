#include "server.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <system_error>
#include <thread>
#include <unistd.h>

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
        workers_.emplace_back(worker);
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
        std::cout << "Acceptor thread finished" << std::endl;
    }

    // Wait for all workers
    for (auto &worker : workers_)
    {
        worker->wait();
    }
    workers_.clear();
}

void Server::shutdown()
{
    shutdown_ = true;
    // Close the listening socket, this will wake up the acceptor thread
    if (socket_fd_ >= 0)
    {
        close(socket_fd_);
        socket_fd_ = -1;
    }

    client_queue_.shutdown();
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
        std::cerr << "Failed to accept connection" << std::endl;
        throw std::system_error(errno, std::system_category(), "Failed to accept connection");
    }
    // Add to queue so a worker can pick it up, locking is handled internally see
    // threadsafe_queue.hpp
    client_queue_.push(client_fd);

    // Log the new connection
    std::string client_ip = inet_ntoa(client_addr.sin_addr);
    uint16_t client_port = ntohs(client_addr.sin_port);

    std::cout << "New connection from " << client_ip << ":" << client_port << std::endl;
    std::cout << "Queue size: " << client_queue_.size() << std::endl;
}

void Server::acceptClients()
{
    try
    {
        while (!shutdown_)
        {
            acceptClient();
        }
    }
    catch (const std::system_error &e)
    {
        if (shutdown_)
        {
            // If shutdown is in progress, break the loop
            return;
        }
        std::cerr << "Exception in acceptClients: " << e.what() << std::endl;
    }
}

void Server::startListening()
{
    if (listen(socket_fd_, 10) < 0)
    {
        throw std::system_error(errno, std::system_category(), "Failed when calling listen");
    }
}