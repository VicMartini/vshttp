#include "threadsafe_queue.hpp"

template <typename T>
void ThreadsafeQueue<T>::push(T value)
{
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(value));
    cv_.notify_all();
}

template <typename T>
T ThreadsafeQueue<T>::pop(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (timeout == std::chrono::milliseconds(0))
    {
        cv_.wait(lock, [this]
                 { return !queue_.empty(); });
    }
    else
    {
        if (!cv_.wait_for(lock, timeout, [this]
                          { return !queue_.empty(); }))
        {
            return T();
        }
    }

    T value = std::move(queue_.front());
    queue_.pop();
    return value;
}

template <typename T>
bool ThreadsafeQueue<T>::empty() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

template <typename T>
size_t ThreadsafeQueue<T>::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

template class ThreadsafeQueue<int>;