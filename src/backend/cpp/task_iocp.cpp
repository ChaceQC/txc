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
            const auto error = GetLastError();
            {
                std::lock_guard lock(mutex_);
                operation->state = task_io_operation::submission_state::completing;
            }
            complete_operation(operation, 0, error);
            return;
        }
        {
            std::lock_guard lock(mutex_);
            operations_.emplace(&operation->overlapped, operation);
            operation->state = task_io_operation::submission_state::submitting;
        }
        wake();
        const auto started = operation->begin();

        bool deliver_completion = false;
        DWORD completion_bytes = 0;
        DWORD completion_error = ERROR_SUCCESS;
        {
            std::lock_guard lock(mutex_);
            const auto found = operations_.find(&operation->overlapped);
            if (operation->completion_received.load())
            {
                if (found != operations_.end())
                {
                    operations_.erase(found);
                }
                operation->state = task_io_operation::submission_state::completing;
                completion_bytes = operation->completion_bytes;
                completion_error = operation->completion_error;
                deliver_completion = true;
            }
            else if (started == ERROR_SUCCESS || started == ERROR_IO_PENDING)
            {
                operation->state = task_io_operation::submission_state::submitted;
            }
            else
            {
                if (found != operations_.end())
                {
                    operations_.erase(found);
                }
                operation->state = task_io_operation::submission_state::completing;
                completion_error = started;
                deliver_completion = true;
            }
        }

        if (deliver_completion)
        {
            complete_operation(operation, completion_bytes, completion_error);
        }
        else
        {
            // 提交期间可能已经收到取消请求；唤醒循环让它在提交后补发取消。
            wake();
        }
    }

    void wake() noexcept
    {
        PostQueuedCompletionStatus(port_, 0, 1, nullptr);
    }

private:
    void complete_operation(const std::shared_ptr<task_io_operation>& operation,
                            DWORD bytes, DWORD error) noexcept
    {
        operation->complete(bytes, error);
        std::lock_guard lock(mutex_);
        operation->state = task_io_operation::submission_state::completed;
    }

    void request_pending_cancellations() noexcept
    {
        std::vector<std::shared_ptr<task_io_operation>> candidates;
        {
            std::lock_guard lock(mutex_);
            for (const auto& [overlapped, operation] : operations_)
            {
                (void)overlapped;
                const auto state = operation->state;
                if ((state == task_io_operation::submission_state::submitting ||
                     state == task_io_operation::submission_state::submitted) &&
                    !operation->completion_received.load() &&
                    (!operation->cancel_requested.load() ||
                     (state == task_io_operation::submission_state::submitted &&
                      !operation->cancel_dispatched)))
                {
                    candidates.push_back(operation);
                }
            }
        }

        for (const auto& operation : candidates)
        {
            bool requested = false;
            {
                std::lock_guard lock(mutex_);
                requested = operation->cancel_requested.load();
            }
            if (!requested && (stopping_ || operation->should_cancel()))
            {
                std::lock_guard lock(mutex_);
                const auto state = operation->state;
                if (state == task_io_operation::submission_state::submitting ||
                    state == task_io_operation::submission_state::submitted)
                {
                    operation->cancel_requested.store(true);
                }
            }

            bool dispatch_cancel = false;
            {
                std::lock_guard lock(mutex_);
                if (operation->state ==
                        task_io_operation::submission_state::submitted &&
                    operation->cancel_requested.load() &&
                    !operation->cancel_dispatched)
                {
                    operation->cancel_dispatched = true;
                    dispatch_cancel = true;
                }
            }
            if (!dispatch_cancel)
            {
                continue;
            }

            if (!CancelIoEx(operation->file, &operation->overlapped))
            {
                const auto error = GetLastError();
                if (error != ERROR_NOT_FOUND)
                {
                    std::lock_guard lock(mutex_);
                    if (operation->state ==
                        task_io_operation::submission_state::submitted)
                    {
                        // 仍由内核使用的缓冲不能提前释放；稍后再尝试发出取消。
                        operation->cancel_dispatched = false;
                    }
                }
                // ERROR_NOT_FOUND 时以随后到达的完成包确定操作结果。
            }
        }
    }

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
                }
            }

            request_pending_cancellations();

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
                bool deliver_completion = false;
                {
                    std::lock_guard lock(mutex_);
                    if (const auto found = operations_.find(overlapped);
                        found != operations_.end())
                    {
                        const auto state = found->second->state;
                        if (state ==
                            task_io_operation::submission_state::submitting)
                        {
                            // 同步成功的 I/O 完成包可能早于 begin() 返回。
                            found->second->completion_received.store(true);
                            found->second->completion_bytes = bytes;
                            found->second->completion_error = error;
                        }
                        else if (state ==
                            task_io_operation::submission_state::submitted)
                        {
                            operation = std::move(found->second);
                            operations_.erase(found);
                            operation->state =
                                task_io_operation::submission_state::completing;
                            deliver_completion = true;
                        }
                    }
                }
                if (deliver_completion)
                {
                    complete_operation(operation, bytes, error);
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
