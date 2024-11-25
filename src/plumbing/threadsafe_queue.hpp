#ifndef THREADSAFE_QUEUE_HPP
#define THREADSAFE_QUEUE_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>

template <typename T> class ThreadsafeQueue
{
  public:
    ThreadsafeQueue() = default;
    ~ThreadsafeQueue() = default;

    void push(T value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_)
        {
            throw std::runtime_error("Cannot push to a shutdown queue");
        }
        queue_.push(std::move(value));
        cv_.notify_all();
    }

    std::optional<T> pop(std::chrono::milliseconds timeout = std::chrono::milliseconds(0))
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (timeout == std::chrono::milliseconds(0))
        {
            cv_.wait(lock, [this] { return shutdown_ || !queue_.empty(); });
        }
        else
        {
            if (!cv_.wait_for(lock, timeout, [this] { return shutdown_ || !queue_.empty(); }))
            {
                return std::nullopt;
            }
        }

        if (shutdown_ && queue_.empty())
        {
            return std::nullopt;
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

    void shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }
        cv_.notify_all();
    }

  private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> shutdown_{false};
};

#endif // THREADSAFE_QUEUE_HPP