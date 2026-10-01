#include "backend/cpp/task_runtime_internal.hpp"
#include "stdlib/task.hpp"
#include "stdlib/error.hpp"

#include <algorithm>
#include <condition_variable>
#include <thread>
#include <vector>

namespace tx_generated
{
namespace
{

using clock_type = std::chrono::steady_clock;

struct timer_entry
{
    clock_type::time_point due;
    std::shared_ptr<task_scope_state> scope;
    std::shared_ptr<task_state> child;
};

class timer_loop
{
public:
    timer_loop() : worker_([this]
    {
        run();
    })
    {
    }

    ~timer_loop()
    {
        {
            std::lock_guard lock(mutex_);
            stopping_ = true;
        }
        wake();
        worker_.join();
    }

    void add(timer_entry entry)
    {
        {
            std::lock_guard lock(mutex_);
            timers_.push_back(std::move(entry));
        }
        wake();
    }

    void wake()
    {
        changed_.notify_all();
    }

private:
    void run()
    {
        std::unique_lock lock(mutex_);
        for (;;)
        {
            const auto now = clock_type::now();
            auto next = now + std::chrono::milliseconds(10);
            for (auto iterator = timers_.begin(); iterator != timers_.end();)
            {
                const bool cancelled = stopping_ || task_cancelled(*iterator->scope);
                if (cancelled || iterator->due <= now)
                {
                    auto entry = std::move(*iterator);
                    iterator = timers_.erase(iterator);
                    complete_task(entry.child, {}, cancelled
                        ? task_error{tx::error_kind::cancelled, "cancelled", "任务已取消", {}}
                        : task_error{});
                }
                else
                {
                    next = std::min(next, iterator->due);
                    ++iterator;
                }
            }
            if (stopping_)
            {
                return;
            }
            if (timers_.empty())
            {
                changed_.wait(lock);
            }
            else
            {
                changed_.wait_until(lock, next);
            }
        }
    }

    std::mutex mutex_;
    std::condition_variable changed_;
    std::vector<timer_entry> timers_;
    bool stopping_ = false;
    std::thread worker_;
};

timer_loop& timers()
{
    static timer_loop instance;
    return instance;
}

} // namespace

void wake_task_timers()
{
    timers().wake();
}

std::shared_ptr<task_state> schedule_task_timer(
    const std::shared_ptr<task_scope_state>& scope, std::int64_t delay_ms)
{
    const auto now = clock_type::now();
    const auto available = std::chrono::duration_cast<std::chrono::milliseconds>(
        clock_type::time_point::max() - now).count();
    if (delay_ms < 0 || delay_ms > available)
    {
        throw runtime_failure({tx::error_kind::runtime, delay_ms < 0 ? "invalid_argument" : "out_of_range",
            "异步计时延迟超出单调时钟范围"});
    }
    auto child = reserve_task(scope);
    try
    {
        timers().add({now + std::chrono::milliseconds(delay_ms), scope, child});
    }
    catch (...)
    {
        discard_task(child);
        throw;
    }
    return child;
}

} // namespace tx_generated
