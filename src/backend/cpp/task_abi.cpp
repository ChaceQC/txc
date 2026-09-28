#include "stdlib/task.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/closure.hpp"

#include <any>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <variant>

namespace
{

using tx_generated::task_scope_state;
using tx_generated::task_state;
using tx_generated::task_error;

struct scope_context_guard
{
    std::shared_ptr<task_scope_state> previous;
    tx_generated::detail::runtime_context& context;
    bool propagation;

    ~scope_context_guard()
    {
        tx_generated::set_current_task_scope(std::move(previous));
        context.propagate_errors = propagation;
    }
};

std::shared_ptr<task_scope_state> require_scope(const void* value)
{
    if (!value)
    {
        throw std::runtime_error("任务作用域为空");
    }
    const auto& handle = std::any_cast<const tx_generated::task_scope_handle&>(
        *static_cast<const std::any*>(value));
    if (!handle.state)
    {
        throw std::runtime_error("任务作用域无效");
    }
    return handle.state;
}

std::shared_ptr<task_state> require_task(const void* value)
{
    if (!value)
    {
        throw std::runtime_error("任务句柄为空");
    }
    const auto& handle = std::any_cast<const tx_generated::task_handle&>(
        *static_cast<const std::any*>(value));
    if (!handle.state)
    {
        throw std::runtime_error("任务句柄无效");
    }
    return handle.state;
}

void replay_error(const task_error& error)
{
    tx_generated::detail::set_error(error.kind, error.code.c_str(),
                                    error.message.c_str());
    tx_generated::detail::current_runtime_context().last_error_stack =
        error.stack;
}

task_error snapshot_error()
{
    const auto& context = tx_generated::detail::current_runtime_context();
    return {context.last_error_kind, context.last_error_code,
            context.last_error, context.last_error_stack};
}

std::shared_ptr<task_scope_state> make_scope(
    std::int64_t max_pending, std::int64_t deadline_ms)
{
    if (max_pending < 1 || max_pending > 1000000 || deadline_ms < -1)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_argument", "任务额度必须为 1～1000000，截止时间须为 -1 或非负毫秒"});
    }
    auto scope = std::make_shared<task_scope_state>();
    scope->maximum = static_cast<std::size_t>(max_pending);
    scope->cancellation = std::make_shared<tx_generated::cancellation_state>();
    if (deadline_ms >= 0)
    {
        const auto now = std::chrono::steady_clock::now();
        const auto available = std::chrono::duration_cast<
            std::chrono::milliseconds>(
                std::chrono::steady_clock::time_point::max() - now).count();
        if (deadline_ms > available)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "out_of_range", "任务截止时间超出单调时钟范围"});
        }
        scope->cancellation->deadline = now +
            std::chrono::milliseconds(deadline_ms);
    }
    return scope;
}

template<class value_type, class output_type>
int scope_value(const void* callback, std::int64_t max_pending,
                std::int64_t deadline_ms, output_type* output) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        if (!callback)
        {
            throw std::runtime_error("任务作用域回调为空");
        }
        const auto& closure = std::any_cast<
            const tx_generated::closure_handle&>(
                *static_cast<const std::any*>(callback));
        auto scope = make_scope(max_pending, deadline_ms);
        auto& context = tx_generated::detail::current_runtime_context();
        scope_context_guard guard{tx_generated::current_task_scope(),
                                  context, context.propagate_errors};
        tx_generated::set_current_task_scope(scope);
        context.propagate_errors = true;
        auto* parameter = tx_generated::detail::make_handle<std::any>(
            tx_generated::task_scope_handle{scope});
        value_type value{};
        try
        {
            const auto target = closure.data().target;
            if constexpr (std::is_same_v<value_type, std::monostate>)
            {
                reinterpret_cast<void (*)(const void*, const void*)>(target)(
                    callback, parameter);
            }
            else if constexpr (std::is_same_v<value_type, std::string> ||
                               std::is_same_v<value_type, std::any>)
            {
                using owner_type = std::unique_ptr<value_type,
                    decltype(&tx_generated::detail::destroy_handle<value_type>)>;
                owner_type returned(reinterpret_cast<value_type* (*)(
                    const void*, const void*)>(target)(callback, parameter),
                    &tx_generated::detail::destroy_handle<value_type>);
                if (returned)
                {
                    value = *returned;
                }
                else if (context.last_error_kind == tx::error_kind::none)
                {
                    throw std::runtime_error("任务作用域结果为空");
                }
            }
            else
            {
                value = reinterpret_cast<value_type (*)(
                    const void*, const void*)>(target)(callback, parameter);
            }
        }
        catch (const std::exception& failure)
        {
            tx_generated::detail::set_error(tx::error_kind::runtime,
                "task_scope_failed", failure.what());
        }
        catch (...)
        {
            tx_generated::detail::set_error(tx::error_kind::runtime,
                "task_scope_failed", "任务作用域回调发生未知错误");
        }
        const auto callback_error = snapshot_error();
        const auto child_error = tx_generated::close_task_scope(scope,
            callback_error.kind != tx::error_kind::none);
        tx_generated::collect_cycles();
        if (callback_error.kind == tx::error_kind::none &&
            child_error.kind != tx::error_kind::none)
        {
            replay_error(child_error);
        }
        if constexpr (std::is_same_v<value_type, std::string> ||
                      std::is_same_v<value_type, std::any>)
        {
            if (output && context.last_error_kind == tx::error_kind::none)
            {
                *output = tx_generated::detail::make_handle<value_type>(
                    std::move(value));
            }
        }
        else if constexpr (!std::is_same_v<value_type, std::monostate>)
        {
            if (output)
            {
                *output = value;
            }
        }
    });
}

template<class value_type, class output_type>
int wait_value(const void* value, output_type* output) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        if (tx_generated::detail::current_runtime_context().collecting)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "gc_in_progress", "循环回收期间不能等待任务"});
        }
        const auto state = require_task(value);
        {
            std::lock_guard lock(state->mutex);
            if (state->observed)
            {
                throw tx_generated::runtime_failure({tx::error_kind::runtime,
                    "invalid_state", "任务结果已经被等待过"});
            }
            state->observed = true;
        }
        tx_generated::wait_task(state);
        tx_generated::collect_cycles();
        std::lock_guard lock(state->mutex);
        if (state->error.kind != tx::error_kind::none)
        {
            replay_error(state->error);
            return;
        }
        if constexpr (std::is_same_v<value_type, std::string> ||
                      std::is_same_v<value_type, std::any>)
        {
            *output = tx_generated::detail::make_handle<value_type>(
                std::get<value_type>(state->result));
        }
        else if constexpr (!std::is_same_v<value_type, std::monostate>)
        {
            *output = std::get<value_type>(state->result);
        }
    });
}

void* new_task_handle(std::shared_ptr<task_state> state, std::int64_t kind,
                      const char* type_name)
{
    return tx_generated::detail::make_handle<std::any>(
        tx_generated::task_handle{std::move(state), kind, type_name});
}

} // namespace

extern "C" int txrt_task_scope_void(const void* callback,
    std::int64_t maximum, std::int64_t deadline_ms) noexcept
{
    return scope_value<std::monostate>(callback, maximum, deadline_ms,
        static_cast<void**>(nullptr));
}

extern "C" int txrt_task_scope_i64(const void* callback,
    std::int64_t maximum, std::int64_t deadline_ms,
    std::int64_t* result) noexcept
{
    return scope_value<std::int64_t>(callback, maximum, deadline_ms, result);
}

extern "C" int txrt_task_scope_f64(const void* callback,
    std::int64_t maximum, std::int64_t deadline_ms, double* result) noexcept
{
    return scope_value<double>(callback, maximum, deadline_ms, result);
}

extern "C" int txrt_task_scope_bool(const void* callback,
    std::int64_t maximum, std::int64_t deadline_ms, bool* result) noexcept
{
    return scope_value<bool>(callback, maximum, deadline_ms, result);
}

extern "C" int txrt_task_scope_str(const void* callback,
    std::int64_t maximum, std::int64_t deadline_ms, void** result) noexcept
{
    return scope_value<std::string>(callback, maximum, deadline_ms, result);
}

extern "C" int txrt_task_scope_value(const void* callback,
    std::int64_t maximum, std::int64_t deadline_ms, void** result) noexcept
{
    return scope_value<std::any>(callback, maximum, deadline_ms, result);
}

extern "C" int txrt_task_spawn(const void* owner, const void* callback,
    std::int64_t kind, const char* type_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        if (tx_generated::detail::current_runtime_context().collecting)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "gc_in_progress", "循环回收期间不能启动任务"});
        }
        if (!callback || !type_name || kind < 0 || kind > 5)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "invalid_argument", "任务回调或返回类型无效"});
        }
        auto copy = *static_cast<const std::any*>(callback);
        (void)std::any_cast<const tx_generated::closure_handle&>(copy);
        *result = new_task_handle(tx_generated::submit_task(
            require_scope(owner), std::move(copy), kind), kind, type_name);
    });
}

extern "C" int txrt_task_spawn_current(const void* callback,
    std::int64_t kind, const char* type_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        if (tx_generated::detail::current_runtime_context().collecting)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "gc_in_progress", "循环回收期间不能启动任务"});
        }
        if (!callback || !type_name || kind < 0 || kind > 5)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "invalid_argument", "异步函数回调或返回类型无效"});
        }
        auto copy = *static_cast<const std::any*>(callback);
        (void)std::any_cast<const tx_generated::closure_handle&>(copy);
        *result = new_task_handle(tx_generated::submit_task(
            tx_generated::current_task_scope(), std::move(copy), kind),
            kind, type_name);
    });
}

extern "C" int txrt_task_wait_void(const void* value) noexcept
{
    return wait_value<std::monostate>(value, static_cast<void**>(nullptr));
}

extern "C" int txrt_task_wait_i64(const void* value,
    std::int64_t* result) noexcept
{
    return wait_value<std::int64_t>(value, result);
}

extern "C" int txrt_task_wait_f64(const void* value, double* result) noexcept
{
    return wait_value<double>(value, result);
}

extern "C" int txrt_task_wait_bool(const void* value, bool* result) noexcept
{
    return wait_value<bool>(value, result);
}

extern "C" int txrt_task_wait_str(const void* value, void** result) noexcept
{
    return wait_value<std::string>(value, result);
}

extern "C" int txrt_task_wait_value(const void* value, void** result) noexcept
{
    return wait_value<std::any>(value, result);
}

extern "C" int txrt_task_cancel(const void* owner, bool* result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        *result = tx_generated::cancel_task_scope(require_scope(owner));
    });
}

extern "C" int txrt_task_token(const void* owner, void** result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::cancel_token{require_scope(owner)->cancellation});
    });
}

extern "C" int txrt_task_current_token(void** result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        auto scope = tx_generated::current_task_scope();
        if (!scope)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "invalid_state", "当前没有活动的任务作用域"});
        }
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::cancel_token{scope->cancellation});
    });
}

extern "C" int txrt_task_after(const void* owner,
    std::int64_t delay_ms, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = new_task_handle(tx_generated::schedule_task_timer(
            require_scope(owner), delay_ms), 0, "task<void>");
    });
}

extern "C" int txrt_task_sleep(std::int64_t delay_ms,
    void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = new_task_handle(tx_generated::schedule_task_timer(
            tx_generated::current_task_scope(), delay_ms), 0, "task<void>");
    });
}
