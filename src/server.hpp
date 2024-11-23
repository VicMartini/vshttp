#ifndef SERVER_HPP
#define SERVER_HPP

#include "threadsafe_queue.hpp"
#include "worker.hpp"
#include <netinet/in.h>
#include <queue>
#include <thread>
#include <vector>
class Server
{
  public:
    explicit Server(int port, size_t num_workers);

    ~Server();
    // TODO: Check the Rule of 5 thing

    void start();
    void wait();
    void shutdown();

  private:
    sockaddr_in createSockAddr() const;
    void bindSocket();
    void startListening();
    void acceptClient();
    void acceptClients();

    const int port_;
    const size_t num_workers_;
    bool shutdown_{false};
    int socket_fd_{-1};
    sockaddr_in addr_{};

    ThreadsafeQueue<int> client_queue_{};

    std::vector<Worker *> workers_{};
    std::vector<std::thread> worker_threads_{};
    std::thread acceptor_thread_{};
};

#endif // RECEIVER_HPP