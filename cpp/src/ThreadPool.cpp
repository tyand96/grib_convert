#include <ThreadPool.hpp>

ThreadPool::ThreadPool(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this] {
            while (!forceTerminate_) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queuedMutex_);
                    taskCondition_.wait(lock, [this] {
                        return stop_ || forceTerminate_ || !tasks_.empty();
                    });

                    if (forceTerminate_) return;
                    if (stop_ && tasks_.empty()) return;

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                if (forceTerminate_) return;
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queuedMutex_);
        stop_ = true;
    }
    taskCondition_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::forceStop() {
    {
        std::unique_lock<std::mutex> lock(queuedMutex_);
        stop_ = true;
        forceTerminate_ = true;

        // Clear the task queue
        std::queue<std::function<void()>> emptyQueue;
        tasks_.swap(emptyQueue);

        activeTaskCount_ = 0;
    }

    taskCondition_.notify_all();
    completionCondition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

bool ThreadPool::areTasksCompleted() const {
    return activeTaskCount_ == 0 && tasks_.empty();
}

void ThreadPool::waitForCompletion() {
    std::unique_lock<std::mutex> lock(queuedMutex_);
    completionCondition_.wait(lock, [this] {
        return activeTaskCount_ == 0 && tasks_.empty();
    });
}