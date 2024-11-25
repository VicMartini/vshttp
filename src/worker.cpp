#include "worker.hpp"

#include "parser/httprequestparser.hpp"
#include "parser/httpresponseparser.hpp"
#include "parser/urlparser.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <unistd.h>

#include <optional>
#include <unordered_map>

std::string getMimeType(const std::string &filename)
{
    /**
     * Returns the MIME type of the file based on its extension.
     */

    // Map of file extensions to MIME types
    static const std::unordered_map<std::string, std::string> mimeTypes = {
        {".html", "text/html"},        {".css", "text/css"},        {".js", "application/javascript"},
        {".json", "application/json"}, {".png", "image/png"},       {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},       {".gif", "image/gif"},       {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},      {".txt", "text/plain"},      {".pdf", "application/pdf"},
        {".zip", "application/zip"},   {".xml", "application/xml"},
    };

    // Find the last dot in the filename
    size_t lastDot = filename.find_last_of('.');
    if (lastDot == std::string::npos)
    {
        return "application/octet-stream"; // Default MIME type
    }

    // Extract the file extension
    std::string extension = filename.substr(lastDot);

    auto it = mimeTypes.find(extension);
    if (it != mimeTypes.end())
    {
        return it->second;
    }

    return "application/octet-stream";
}

Worker::Worker(size_t id) : id_(id)
{
}

void Worker::start(ThreadsafeQueue<int> &client_queue, std::atomic<bool> &shutdown)
{
    thread_ = std::thread([this, &client_queue, &shutdown]() { this->serviceClients(client_queue, shutdown); });
}

void Worker::serviceClients(ThreadsafeQueue<int> &client_queue, std::atomic<bool> &shutdown)
{
    while (!shutdown)
    {
        // Blocking pop, see threadsafe_queue.hpp
        std::optional<int> clientFd = client_queue.pop();
        if (!clientFd)
        {
            continue;
        }

        handleRequest(*clientFd);
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
        logError("Failed to read from client socket");
        return;
    }

    auto request = parseRequest(buffer, bytes_read);
    if (!request)
    {
        return;
    }

    // TODO: Use something robust, like using std::filesystem::path, this is very error prone
    std::string files_dir = std::getenv("FILES_DIR");
    // Remove trailing slash if present
    if (files_dir.back() == '/')
    {
        files_dir = files_dir.substr(0, files_dir.size() - 1);
    }
    const std::string file_path = files_dir + request->uri;

    auto file_content = readFileContent(file_path);
    if (!file_content)
    {
        sendErrorResponse(client_fd, 404, "Not Found");
        return;
    }

    sendResponse(client_fd, *request, *file_content);
}

std::optional<httpparser::Request> Worker::parseRequest(const char *buffer, ssize_t bytes_read)
{
    httpparser::HttpRequestParser parser;
    httpparser::Request request;
    auto res = parser.parse(request, buffer, buffer + bytes_read);

    if (res == httpparser::HttpRequestParser::ParsingError)
    {
        logError("Failed to parse HTTP request");
        return std::nullopt;
    }
    else if (res == httpparser::HttpRequestParser::ParsingIncompleted)
    {
        logError("Incomplete HTTP request");
        return std::nullopt;
    }

    std::cout << "URL: " << request.uri << std::endl;
    std::cout << "Received request: " << request.inspect() << std::endl;
    return request;
}

std::optional<std::vector<char>> Worker::readFileContent(const std::string &file_path)
{
    std::ifstream file(file_path);
    if (!file.is_open())
    {
        return std::nullopt;
    }

    return std::vector<char>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void Worker::sendErrorResponse(int client_fd, int statusCode, const std::string &status)
{
    httpparser::Response response;
    response.versionMajor = 1;
    response.versionMinor = 1;
    response.statusCode = statusCode;
    response.status = status;
    std::string responseStr = response.inspect();
    write(client_fd, responseStr.c_str(), responseStr.size());
    close(client_fd);
}

void Worker::sendResponse(int client_fd, const httpparser::Request &request, const std::vector<char> &file_content)
{
    httpparser::Response response;
    response.versionMajor = 1;
    response.versionMinor = 1;
    response.statusCode = 200;
    response.status = "OK";
    response.keepAlive = request.keepAlive;

    // Add headers
    httpparser::Response::HeaderItem contentTypeHeader;
    contentTypeHeader.name = "Content-Type";
    contentTypeHeader.value = getMimeType(request.uri);
    response.headers.push_back(contentTypeHeader);

    httpparser::Response::HeaderItem contentLengthHeader;
    contentLengthHeader.name = "Content-Length";
    contentLengthHeader.value = std::to_string(file_content.size());
    response.headers.push_back(contentLengthHeader);

    // Add content
    response.content.assign(file_content.begin(), file_content.end());

    // Serialize response
    std::string responseStr = response.inspect();

    // Send response
    ssize_t bytes_written = write(client_fd, responseStr.c_str(), responseStr.size());
    if (bytes_written < 0)
    {
        logError("Failed to write to client socket");
    }

    // Close the client socket
    if (close(client_fd) < 0)
    {
        logError("Failed to close client socket");
    }
}

void Worker::logError(const std::string &message)
{
    std::cerr << message << ": " << strerror(errno) << std::endl;
}