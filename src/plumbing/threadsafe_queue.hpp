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

    void push(T value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(value));
        cv_.notify_all();
    }

    T pop(std::chrono::milliseconds timeout = std::chrono::milliseconds(0))
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (timeout == std::chrono::milliseconds(0))
        {
            cv_.wait(lock, [this] { return !queue_.empty(); });
        }
        else
        {
            if (!cv_.wait_for(lock, timeout, [this] { return !queue_.empty(); }))
            {
                return T();
            }
        }

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

  private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

#endif // THREADSAFE_QUEUE_HPP