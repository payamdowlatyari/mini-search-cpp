#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

namespace search {

/// A fixed-size thread pool that executes submitted tasks concurrently.
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    // Non-copyable, non-movable.
    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// Submit a callable and return a future for its result.
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    /// Number of worker threads.
    size_t size() const { return workers_.size(); }

private:
    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        mutex_;
    std::condition_variable           cv_;
    bool                              stop_{false};
};

// ---------------------------------------------------------------------------
// template implementation
// ---------------------------------------------------------------------------

template<typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
    using ReturnType = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    auto future = task->get_future();
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (stop_) throw std::runtime_error("ThreadPool has been stopped");
        tasks_.emplace([task]() { (*task)(); });
    }
    cv_.notify_one();
    return future;
}

} // namespace search
