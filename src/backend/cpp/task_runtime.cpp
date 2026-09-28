#include "stdlib/task.hpp"

#include "backend/cpp/cycle_gc.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/task_executor.hpp"
#include "backend/cpp/task_runtime_internal.hpp"
#include "stdlib/closure.hpp"

#include <algorithm>
#include <any>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace tx_generated
{

task_state::task_state()
{
#ifdef _WIN32
    completed_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!completed_event)
    {
        throw runtime_failure({tx::error_kind::runtime,
            "task_event_failed", "无法创建任务完成事件"});
    }
#endif
}

task_state::~task_state()
{
#ifdef _WIN32
    CloseHandle(static_cast<HANDLE>(completed_event));
#endif
}

namespace
{

using clock_type = std::chrono::steady_clock;

thread_local std::shared_ptr<task_scope_state> active_scope;

task_error runtime_error(const detail::runtime_context& context)
{
    return {context.last_error_kind, context.last_error_code,
            context.last_error, context.last_error_stack};
}

task_error cancelled_error()
{
    return {tx::error_kind::cancelled, "cancelled", "任务已取消", {}};
}

void finish_task(const std::shared_ptr<task_state>& state,
                 task_result result, task_error error)
{
    {
        std::lock_guard lock(state->mutex);
        state->result = std::move(result);
        state->error = std::move(error);
        state->completed = true;
    }
#ifdef _WIN32
    SetEvent(static_cast<HANDLE>(state->completed_event));
#else
    state->completed_signal.notify_all();
#endif
}

void run_callback(const std::shared_ptr<task_scope_state>& scope,
                  const std::shared_ptr<task_state>& state,
                  std::any callback, std::int64_t kind) noexcept
{
    // 帮助执行嵌套任务时，每个任务必须有独立的错误栈和句柄登记链。
    detail::runtime_context context;
    auto* previous_context = detail::thread_context;
    detail::thread_context = &context;
    context.propagate_errors = true;
    const auto previous_scope = active_scope;
    active_scope = scope;
    task_result result;
    task_error error;
    {
        concurrent_execution_scope execution;
        if (task_cancelled(*scope))
        {
            error = cancelled_error();
        }
        else
        {
            try
            {
                const auto& closure = std::any_cast<const closure_handle&>(callback);
                const auto target = closure.data().target;
                result = invoke_concurrent_callback(target, &callback, kind);
            }
            catch (const std::exception& failure)
            {
                detail::set_error(tx::error_kind::runtime,
                                  "task_failed", failure.what());
            }
            catch (...)
            {
                detail::set_error(tx::error_kind::runtime,
                                  "task_failed", "任务发生未知错误");
            }
            error = runtime_error(context);
        }
        callback.reset();
        detail::cleanup_live_handles();
        try
        {
            collect_cycles();
        }
        catch (...)
        {
            if (error.kind == tx::error_kind::none)
            {
                error = {tx::error_kind::runtime, "gc_failed",
                         "任务退出时循环回收失败", {}};
            }
        }
    }
    active_scope = previous_scope;
    detail::thread_context = previous_context;
    finish_task(state, std::move(result), std::move(error));
}

std::shared_ptr<task_state> add_child(
    const std::shared_ptr<task_scope_state>& scope)
{
    if (!scope)
    {
        throw runtime_failure({tx::error_kind::runtime,
            "invalid_state", "当前没有活动的任务作用域"});
    }
    auto child = std::make_shared<task_state>();
    std::lock_guard lock(scope->mutex);
    if (scope->closed)
    {
        throw runtime_failure({tx::error_kind::runtime,
            "invalid_state", "任务作用域已经结束"});
    }
    // 已完成且无需在退出时交付错误的状态可由任务句柄自行持有。
    std::erase_if(scope->children, [](const auto& existing)
    {
        std::lock_guard child_lock(existing->mutex);
        return existing->completed &&
            (existing->observed || existing->error.kind == tx::error_kind::none);
    });
    if (scope->children.size() >= scope->maximum)
    {
        throw runtime_failure({tx::error_kind::runtime,
            "task_limit", "任务作用域的待完成任务或未观察错误已达上限"});
    }
    scope->children.push_back(child);
    return child;
}

} // namespace

std::shared_ptr<task_state> reserve_task(
    const std::shared_ptr<task_scope_state>& scope)
{
    return add_child(scope);
}

void discard_task(const std::shared_ptr<task_state>& child)
{
    {
        std::lock_guard lock(child->mutex);
        child->observed = true;
    }
    // 提交失败已经交给调用方；保留一个已完成记录，避免关闭作用域时并发改写子任务表。
    finish_task(child, {}, {});
}

void complete_task(const std::shared_ptr<task_state>& child,
                   task_result result, task_error error)
{
    finish_task(child, std::move(result), std::move(error));
}

std::shared_ptr<task_scope_state> current_task_scope()
{
    return active_scope;
}

void set_current_task_scope(std::shared_ptr<task_scope_state> scope)
{
    active_scope = std::move(scope);
}

bool task_cancelled(task_scope_state& scope)
{
    auto& cancellation = *scope.cancellation;
    std::lock_guard lock(cancellation.mutex);
    return cancellation.cancelled ||
        (cancellation.deadline && clock_type::now() >= *cancellation.deadline);
}

bool cancel_task_scope(const std::shared_ptr<task_scope_state>& scope)
{
    bool changed;
    {
        std::lock_guard lock(scope->cancellation->mutex);
        changed = !scope->cancellation->cancelled;
        scope->cancellation->cancelled = true;
    }
    scope->cancellation->changed.notify_all();
    wake_task_timers();
    return changed;
}

std::shared_ptr<task_state> submit_task(
    const std::shared_ptr<task_scope_state>& scope, std::any callback,
    std::int64_t kind)
{
    auto child = add_child(scope);
    try
    {
        enqueue_task([scope, child, callback = std::move(callback), kind]() mutable
        {
            run_callback(scope, child, std::move(callback), kind);
        });
    }
    catch (...)
    {
        discard_task(child);
        throw;
    }
    return child;
}

void wait_task(const std::shared_ptr<task_state>& state)
{
    for (;;)
    {
        {
            std::unique_lock lock(state->mutex);
            if (state->completed)
            {
                return;
            }
            if (!on_task_executor_thread())
            {
#ifdef _WIN32
                lock.unlock();
                WaitForSingleObject(static_cast<HANDLE>(state->completed_event),
                                    INFINITE);
#else
                state->completed_signal.wait(lock,
                    [&] { return state->completed; });
#endif
                return;
            }
        }
        // 工作者等待子任务时执行队列中的任务，避免固定线程池自锁。
        if (!help_task_executor())
        {
#ifdef _WIN32
            WaitForSingleObject(static_cast<HANDLE>(state->completed_event), 1);
#else
            std::unique_lock lock(state->mutex);
            state->completed_signal.wait_for(lock,
                std::chrono::milliseconds(1));
#endif
        }
    }
}

task_error close_task_scope(const std::shared_ptr<task_scope_state>& scope,
                            bool cancel_first)
{
    {
        std::lock_guard lock(scope->mutex);
        scope->closed = true;
    }
    if (cancel_first)
    {
        (void)cancel_task_scope(scope);
    }
    task_error first;
    for (const auto& child : scope->children)
    {
        wait_task(child);
        std::lock_guard lock(child->mutex);
        if (!child->observed && first.kind == tx::error_kind::none &&
            child->error.kind != tx::error_kind::none)
        {
            first = child->error;
        }
    }
    return first;
}

} // namespace tx_generated
