#ifndef WORKER_HPP
#define WORKER_HPP

#include <queue>
#include <thread>
#include "threadsafe_queue.hpp"
class Worker
{
public:
    explicit Worker(size_t id);
    void start(ThreadsafeQueue<int> &client_queue, bool &shutdown);
    void wait();

private:
    size_t id_;
    std::thread thread_;
    void handleRequest(int client_fd);
    void serviceClients(ThreadsafeQueue<int> &client_queue, bool &shutdown);
};

#endif // WORKER_HPP
