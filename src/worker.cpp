#include "worker.hpp"

#include "parser/httprequestparser.hpp"
#include "parser/httpresponseparser.hpp"
#include <cstring>
#include <iostream>
#include <unistd.h>

Worker::Worker(size_t id) : id_(id)
{
}

void Worker::start(ThreadsafeQueue<int> &client_queue, bool &shutdown)
{
    thread_ = std::thread([this, &client_queue, &shutdown]() { this->serviceClients(client_queue, shutdown); });
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
    char buffer[4096];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));
    if (bytes_read < 0)
    {
        std::cerr << "Failed to read from client socket: " << strerror(errno) << std::endl;
        return;
    }

    httpparser::HttpRequestParser parser;
    httpparser::Request request;
    httpparser::HttpRequestParser::ParseResult res = parser.parse(request, buffer, buffer + bytes_read);

    if (res == httpparser::HttpRequestParser::ParsingError)
    {
        std::cerr << "Failed to parse HTTP request" << std::endl;
        return;
    }
    else if (res == httpparser::HttpRequestParser::ParsingIncompleted)
    {
        std::cerr << "Incomplete HTTP request" << std::endl;
        return;
    }

    std::cout << "Received request: " << request.inspect() << std::endl;

    // Create a response
    httpparser::Response response;
    response.versionMajor = 1;
    response.versionMinor = 1;
    response.statusCode = 200;
    response.status = "OK";
    response.keepAlive = request.keepAlive;

    // Add headers
    httpparser::Response::HeaderItem contentTypeHeader;
    contentTypeHeader.name = "Content-Type";
    contentTypeHeader.value = "text/plain";
    response.headers.push_back(contentTypeHeader);

    httpparser::Response::HeaderItem contentLengthHeader;
    contentLengthHeader.name = "Content-Length";
    contentLengthHeader.value = "2";
    response.headers.push_back(contentLengthHeader);

    // Add content
    response.content.assign({'O', 'K'});

    // Serialize response
    std::string responseStr = response.inspect();

    // Send response
    ssize_t bytes_written = write(client_fd, responseStr.c_str(), responseStr.size());
    if (bytes_written < 0)
    {
        std::cerr << "Failed to write to client socket: " << strerror(errno) << std::endl;
    }

    // Close the client socket
    if (close(client_fd) < 0)
    {
        std::cerr << "Failed to close client socket: " << strerror(errno) << std::endl;
    }
}