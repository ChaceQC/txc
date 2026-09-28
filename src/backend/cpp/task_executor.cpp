#include "backend/cpp/task_executor.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace tx_generated
{
namespace
{

thread_local bool in_task_worker = false;

class worker_pool
{
public:
    worker_pool()
    {
        const auto count = std::clamp(std::thread::hardware_concurrency(), 2u, 8u);
#ifdef _WIN32
        ready_handle_ = CreateSemaphoreW(nullptr, 0, 2048, nullptr);
        if (!ready_handle_)
        {
            throw runtime_failure({tx::error_kind::runtime,
                "task_queue_failed", "无法创建任务队列信号量"});
        }
#endif
        try
        {
            for (unsigned index = 0; index < count; ++index)
            {
                workers_.emplace_back([this]
                {
                    in_task_worker = true;
                    while (run_one(true))
                    {
                    }
                    in_task_worker = false;
                });
            }
        }
        catch (...)
        {
            stop();
            throw;
        }
    }

    ~worker_pool()
    {
        stop();
    }

    void submit(std::function<void()> work)
    {
        {
            std::lock_guard lock(mutex_);
            if (stopping_ || queue_.size() >= 1024)
            {
                throw runtime_failure({tx::error_kind::runtime,
                    "task_queue_full", "任务执行队列已满"});
            }
            queue_.push_back(std::move(work));
        }
#ifdef _WIN32
        ReleaseSemaphore(ready_handle_, 1, nullptr);
#else
        ready_.notify_one();
#endif
    }

    bool run_one(bool wait)
    {
        std::function<void()> work;
        {
#ifdef _WIN32
            const auto signal = WaitForSingleObject(ready_handle_,
                wait ? INFINITE : 0);
            if (signal != WAIT_OBJECT_0)
            {
                return false;
            }
#endif
            std::unique_lock lock(mutex_);
#ifndef _WIN32
            if (wait)
            {
                ready_.wait(lock, [&]
                {
                    return stopping_ || !queue_.empty();
                });
            }
#endif
            if (queue_.empty())
            {
                return false;
            }
            work = std::move(queue_.front());
            queue_.pop_front();
        }
        work();
        return true;
    }

private:
    void stop() noexcept
    {
        {
            std::lock_guard lock(mutex_);
            stopping_ = true;
        }
#ifdef _WIN32
        if (ready_handle_)
        {
            ReleaseSemaphore(ready_handle_,
                static_cast<LONG>(workers_.size()), nullptr);
        }
#else
        ready_.notify_all();
#endif
        for (auto& worker : workers_)
        {
            worker.join();
        }
#ifdef _WIN32
        if (ready_handle_)
        {
            CloseHandle(ready_handle_);
            ready_handle_ = nullptr;
        }
#endif
    }

    std::mutex mutex_;
#ifdef _WIN32
    HANDLE ready_handle_ = nullptr;
#else
    std::condition_variable ready_;
#endif
    std::deque<std::function<void()>> queue_;
    std::vector<std::thread> workers_;
    bool stopping_ = false;
};

worker_pool& pool()
{
    static worker_pool instance;
    return instance;
}

} // namespace

void enqueue_task(std::function<void()> work)
{
    pool().submit(std::move(work));
}

bool help_task_executor()
{
    return pool().run_one(false);
}

bool on_task_executor_thread() noexcept
{
    return in_task_worker;
}

} // namespace tx_generated
