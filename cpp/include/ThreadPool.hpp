#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <cstddef> // For size_t
#include <vector>
#include <queue>
#include <thread>
#include <future>

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    void waitForCompletion();
    void forceStop();
    bool areTasksCompleted() const;

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queuedMutex_);
            if (stop_) {
                throw std::runtime_error("Cannot enqueue task on stopped ThreadPool");
            }

            activeTaskCount_++;

            tasks_.push([this, task]() {
                try {
                    (*task)();
                } catch (...) {
                    // Exceptions will be propogated through the future.
                }

                size_t remaining = --activeTaskCount_;
                if (remaining == 0) {
                    completionCondition_.notify_all();
                }
            });
        }
        taskCondition_.notify_one();
        return res;
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queuedMutex_;
    std::condition_variable taskCondition_;
    std::condition_variable completionCondition_;
    bool stop_ = false;
    std::atomic<size_t> activeTaskCount_{0};
    std::atomic<bool> forceTerminate_{false};

};

#endif // THREAD_POOL_HPP