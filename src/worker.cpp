#include "worker.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>

Worker::Worker(size_t id) : id_(id) {}

void Worker::start(ThreadsafeQueue<int> &client_queue, bool &shutdown)
{
    thread_ = std::thread([this, &client_queue, &shutdown]()
                          { this->serviceClients(client_queue, shutdown); });
}

void Worker::serviceClients(ThreadsafeQueue<int> &client_queue, bool &shutdown)
{
    while (!shutdown)
    {
        // Blocking pop, see threadsafe_queue.hpp
        int client_fd = client_queue.pop();
        std::cout << "Worker " << id_ << " handling request " << client_fd << std::endl;
        handleRequest(client_fd);
    }
}

void Worker::wait()
{
    if (thread_.joinable())
    {
        thread_.join();
    }
}

void Worker::handleRequest(int client_fd)
{
    std::cout << "Worker " << id_ << " handling request" << std::endl;
    const char *message = "I'm a teapot!";
    const char *response = "HTTP/1.1 418 I'm a teapot\r\n"
                           "Content-Length: 13\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "I'm a teapot!";

    size_t total_length = strlen(response);
    size_t bytes_sent = 0;

    // Keep writing until all bytes are sent or an error occurs
    while (bytes_sent < total_length)
    {
        ssize_t bytes_written = write(client_fd, response + bytes_sent,
                                      total_length - bytes_sent);
        if (bytes_written < 0)
        {
            if (errno == EINTR)
            {
                // System call was interrupted, try again
                continue;
            }
            std::cerr << "Failed to write to client socket: "
                      << strerror(errno) << std::endl;
            break;
        }
        bytes_sent += bytes_written;
    }

    // Close the client socket
    if (close(client_fd) < 0)
    {
        std::cerr << "Failed to close client socket: "
                  << strerror(errno) << std::endl;
    }
}