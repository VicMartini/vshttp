#ifndef THREADSAFE_QUEUE_HPP
#define THREADSAFE_QUEUE_HPP

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T> class ThreadsafeQueue
{
  public:
    ThreadsafeQueue() = default;
    ~ThreadsafeQueue() = default;

    void push(T value);
    T pop(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    bool empty() const;
    size_t size() const;

  private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

#endif // THREADSAFE_QUEUE_HPP