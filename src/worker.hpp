#ifndef WORKER_HPP
#define WORKER_HPP

#include "parser/httprequestparser.hpp"
#include "parser/httpresponseparser.hpp"
#include "plumbing/threadsafe_queue.hpp"
#include <optional>
#include <string>
#include <thread>
#include <vector>

class Worker
{
  public:
    explicit Worker(size_t id);
    void start(ThreadsafeQueue<int> &client_queue, std::atomic<bool> &shutdown);
    void wait();

  private:
    size_t id_;
    std::thread thread_;
    void handleRequest(int client_fd);
    void serviceClients(ThreadsafeQueue<int> &client_queue, std::atomic<bool> &shutdown);

    std::optional<httpparser::Request> parseRequest(const char *buffer, ssize_t bytes_read);
    std::optional<std::vector<char>> readFileContent(const std::string &file_path);
    void sendErrorResponse(int client_fd, int statusCode, const std::string &status);
    void sendResponse(int client_fd, const httpparser::Request &request, const std::vector<char> &file_content);
    void logError(const std::string &message);
};

#endif // WORKER_HPP
