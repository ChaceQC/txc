#include "backend/cpp/task_runtime_internal.hpp"
#include "backend/cpp/task_iocp.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace tx_generated
{
#ifdef _WIN32
task_io_operation::~task_io_operation()
{
    close_file();
}

void task_io_operation::close_file() noexcept
{
    if (file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file);
        file = INVALID_HANDLE_VALUE;
    }
}

bool task_io_operation::should_cancel() const
{
    if (task_cancelled(*scope))
    {
        return true;
    }
    std::lock_guard lock(token->mutex);
    return token->cancelled ||
        (token->deadline && std::chrono::steady_clock::now() >=
            *token->deadline);
}
#endif

namespace
{

using clock_type = std::chrono::steady_clock;

#ifdef _WIN32
struct timer_entry
{
    clock_type::time_point due;
    std::shared_ptr<task_scope_state> scope;
    std::shared_ptr<task_state> child;
};

class completion_loop
{
public:
    completion_loop()
    {
        port_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
        if (!port_)
        {
            throw runtime_failure({tx::error_kind::runtime,
                "iocp_unavailable", "无法创建 Windows IOCP 事件循环"});
        }
        try
        {
            worker_ = std::thread([this]
            {
                run();
            });
        }
        catch (...)
        {
            CloseHandle(port_);
            throw;
        }
    }

    ~completion_loop()
    {
        stopping_ = true;
        wake();
        worker_.join();
        CloseHandle(port_);
    }

    void add(timer_entry entry)
    {
        {
            std::lock_guard lock(mutex_);
            timers_.push_back(std::move(entry));
        }
        wake();
    }

    void add_io(std::shared_ptr<task_io_operation> operation)
    {
        if (!CreateIoCompletionPort(operation->file, port_, 0, 0))
        {
            operation->complete(0, GetLastError());
            return;
        }
        {
            std::lock_guard lock(mutex_);
            operations_.emplace(&operation->overlapped, operation);
        }
        const auto started = operation->begin();
        if (started != ERROR_SUCCESS && started != ERROR_IO_PENDING)
        {
            {
                std::lock_guard lock(mutex_);
                operations_.erase(&operation->overlapped);
            }
            operation->complete(0, started);
            return;
        }
        wake();
    }

    void wake() noexcept
    {
        PostQueuedCompletionStatus(port_, 0, 1, nullptr);
    }

private:
    void run() noexcept
    {
        while (true)
        {
            std::optional<timer_entry> ready;
            DWORD timeout = INFINITE;
            {
                std::lock_guard lock(mutex_);
                if (stopping_ && timers_.empty() && operations_.empty())
                {
                    break;
                }
                const auto now = clock_type::now();
                for (auto current = timers_.begin(); current != timers_.end();
                     ++current)
                {
                    if (stopping_ || current->due <= now ||
                        task_cancelled(*current->scope))
                    {
                        ready = std::move(*current);
                        timers_.erase(current);
                        break;
                    }
                    timeout = std::min(timeout, milliseconds_until(
                        current->due, now));
                    std::lock_guard cancellation_lock(
                        current->scope->cancellation->mutex);
                    if (current->scope->cancellation->deadline)
                    {
                        timeout = std::min(timeout, milliseconds_until(
                            *current->scope->cancellation->deadline, now));
                    }
                }
                if (!operations_.empty())
                {
                    timeout = std::min<DWORD>(timeout, 10);
                    for (const auto& [overlapped, operation] : operations_)
                    {
                        if (!operation->cancel_requested &&
                            (stopping_ || operation->should_cancel()))
                        {
                            operation->cancel_requested = true;
                            CancelIoEx(operation->file, overlapped);
                        }
                    }
                }
            }
            if (ready)
            {
                const auto cancelled = stopping_ ||
                    task_cancelled(*ready->scope);
                complete_task(ready->child, {}, cancelled
                    ? task_error{tx::error_kind::cancelled, "cancelled",
                                 "任务已取消", {}}
                    : task_error{});
                continue;
            }
            DWORD bytes = 0;
            ULONG_PTR key = 0;
            OVERLAPPED* overlapped = nullptr;
            const auto success = GetQueuedCompletionStatus(
                port_, &bytes, &key, &overlapped, timeout);
            const auto error = success ? ERROR_SUCCESS : GetLastError();
            if (overlapped)
            {
                std::shared_ptr<task_io_operation> operation;
                {
                    std::lock_guard lock(mutex_);
                    if (const auto found = operations_.find(overlapped);
                        found != operations_.end())
                    {
                        operation = std::move(found->second);
                        operations_.erase(found);
                    }
                }
                if (operation)
                {
                    operation->complete(bytes, error);
                }
            }
        }
    }

    static DWORD milliseconds_until(clock_type::time_point due,
                                    clock_type::time_point now) noexcept
    {
        const auto remaining = std::chrono::duration_cast<
            std::chrono::milliseconds>(due - now).count();
        return static_cast<DWORD>(std::clamp<std::int64_t>(
            remaining, 1, MAXDWORD - 1));
    }

    HANDLE port_ = nullptr;
    std::mutex mutex_;
    std::vector<timer_entry> timers_;
    std::unordered_map<OVERLAPPED*,
        std::shared_ptr<task_io_operation>> operations_;
    std::thread worker_;
    std::atomic<bool> stopping_ = false;
};

completion_loop& iocp_loop()
{
    static completion_loop instance;
    return instance;
}
#endif

} // namespace

#ifdef _WIN32
void submit_io_operation(std::shared_ptr<task_io_operation> operation)
{
    iocp_loop().add_io(std::move(operation));
}
#endif

void wake_task_timers()
{
#ifdef _WIN32
    iocp_loop().wake();
#endif
}

std::shared_ptr<task_state> schedule_task_timer(
    const std::shared_ptr<task_scope_state>& scope, std::int64_t delay_ms)
{
    if (delay_ms < 0)
    {
        throw runtime_failure({tx::error_kind::runtime,
            "invalid_argument", "异步计时延迟不能为负"});
    }
#ifdef _WIN32
    auto child = reserve_task(scope);
    try
    {
        const auto now = clock_type::now();
        const auto available = std::chrono::duration_cast<
            std::chrono::milliseconds>(clock_type::time_point::max() -
                                        now).count();
        if (delay_ms > available)
        {
            throw runtime_failure({tx::error_kind::runtime,
                "out_of_range", "异步计时延迟超出单调时钟范围"});
        }
        iocp_loop().add({now + std::chrono::milliseconds(delay_ms), scope, child});
    }
    catch (...)
    {
        discard_task(child);
        throw;
    }
    return child;
#else
    (void)scope;
    throw runtime_failure({tx::error_kind::runtime,
        "iocp_unavailable", "当前平台不支持 Windows IOCP 事件循环"});
#endif
}

} // namespace tx_generated
