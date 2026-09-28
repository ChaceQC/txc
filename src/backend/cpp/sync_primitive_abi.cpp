#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/runtime_context.hpp"
#include "stdlib/cancellation.hpp"
#include "stdlib/closure.hpp"
#include "stdlib/synchronization.hpp"

#include <algorithm>
#include <any>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <thread>

namespace
{

using clock_type = std::chrono::steady_clock;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

template<class value_type>
std::shared_ptr<value_type> state_of(const void* value)
{
    return std::any_cast<const std::shared_ptr<value_type>&>(
        *static_cast<const std::any*>(value));
}

clock_type::time_point limit_from_ms(std::int64_t timeout_ms)
{
    if (timeout_ms < -1)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_argument", "等待超时只能为 -1 或非负毫秒"});
    }
    if (timeout_ms == -1)
    {
        return clock_type::time_point::max();
    }
    const auto now = clock_type::now();
    const auto available = std::chrono::duration_cast<std::chrono::milliseconds>(
        clock_type::time_point::max() - now).count();
    if (timeout_ms > available)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "out_of_range", "等待超时超出单调时钟范围"});
    }
    return now + std::chrono::milliseconds(timeout_ms);
}

void check_cancel(const void* value)
{
    const auto& token = std::any_cast<const tx_generated::cancel_token&>(
        *static_cast<const std::any*>(value));
    if (!token.state)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_state", "取消令牌已失效"});
    }
    std::lock_guard lock(token.state->mutex);
    if (token.state->cancelled)
    {
        throw tx_generated::runtime_failure({tx::error_kind::cancelled,
            "cancelled", "等待操作已取消"});
    }
    if (token.state->deadline &&
        clock_type::now() >= *token.state->deadline)
    {
        throw tx_generated::runtime_failure({tx::error_kind::cancelled,
            "deadline_exceeded", "等待操作已超过截止时间"});
    }
}

clock_type::time_point next_wake(clock_type::time_point limit)
{
    return std::min(limit, clock_type::now() + std::chrono::milliseconds(10));
}

template<class value_type>
int wait_condition(const void* signal, const void* guard,
                   std::int64_t timeout_ms, const void* token,
                   bool* result) noexcept
{
    return invoke_checked([&]
    {
        auto condition = state_of<tx_generated::condition_state>(signal);
        auto held = state_of<tx_generated::mutex_guard_state<value_type>>(guard);
        if (!held->lock.owns_lock())
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "invalid_state", "等待需要仍持有的 mutex_guard"});
        }
        const auto limit = limit_from_ms(timeout_ms);
        while (true)
        {
            check_cancel(token);
            if (clock_type::now() >= limit)
            {
                *result = false;
                return;
            }
            if (condition->changed.wait_until(held->lock,
                    next_wake(limit)) == std::cv_status::no_timeout)
            {
                // 条件变量允许伪唤醒，调用方须在持锁状态重新检查条件。
                *result = true;
                return;
            }
        }
    });
}

int acquire_semaphore(const void* value, std::int64_t timeout_ms,
                      const void* token, bool* result) noexcept
{
    return invoke_checked([&]
    {
        auto state = state_of<tx_generated::semaphore_state>(value);
        const auto limit = limit_from_ms(timeout_ms);
        std::unique_lock lock(state->mutex);
        while (true)
        {
            if (state->count > 0)
            {
                --state->count;
                *result = true;
                return;
            }
            check_cancel(token);
            if (clock_type::now() >= limit)
            {
                *result = false;
                return;
            }
            state->changed.wait_until(lock, next_wake(limit));
        }
    });
}

} // namespace

extern "C" int txrt_sync_new_condition(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(
            std::make_shared<tx_generated::condition_state>());
    });
}

extern "C" int txrt_sync_notify_one(const void* value) noexcept
{
    return invoke_checked([&]
    {
        auto state = state_of<tx_generated::condition_state>(value);
        state->changed.notify_one();
    });
}

extern "C" int txrt_sync_notify_all(const void* value) noexcept
{
    return invoke_checked([&]
    {
        auto state = state_of<tx_generated::condition_state>(value);
        state->changed.notify_all();
    });
}

extern "C" int txrt_sync_wait_i64(const void* signal, const void* guard,
    std::int64_t timeout_ms, const void* token, bool* result) noexcept
{
    return wait_condition<std::int64_t>(signal, guard, timeout_ms, token, result);
}

extern "C" int txrt_sync_wait_f64(const void* signal, const void* guard,
    std::int64_t timeout_ms, const void* token, bool* result) noexcept
{
    return wait_condition<double>(signal, guard, timeout_ms, token, result);
}

extern "C" int txrt_sync_wait_bool(const void* signal, const void* guard,
    std::int64_t timeout_ms, const void* token, bool* result) noexcept
{
    return wait_condition<bool>(signal, guard, timeout_ms, token, result);
}

extern "C" int txrt_sync_wait_str(const void* signal, const void* guard,
    std::int64_t timeout_ms, const void* token, bool* result) noexcept
{
    return wait_condition<std::string>(signal, guard, timeout_ms, token, result);
}

extern "C" int txrt_sync_wait_value(const void* signal, const void* guard,
    std::int64_t timeout_ms, const void* token, bool* result) noexcept
{
    return wait_condition<std::any>(signal, guard, timeout_ms, token, result);
}

extern "C" int txrt_sync_new_semaphore(std::int64_t initial,
    std::int64_t maximum, void** result) noexcept
{
    return invoke_checked([&]
    {
        if (maximum <= 0 || initial < 0 || initial > maximum)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "invalid_argument", "信号量初值或上限无效"});
        }
        auto state = std::make_shared<tx_generated::semaphore_state>();
        state->count = initial;
        state->maximum = maximum;
        *result = make_handle<std::any>(std::move(state));
    });
}

extern "C" int txrt_sync_acquire(const void* value,
    std::int64_t timeout_ms, const void* token, bool* result) noexcept
{
    return acquire_semaphore(value, timeout_ms, token, result);
}

extern "C" int txrt_sync_release(const void* value,
    std::int64_t permits) noexcept
{
    return invoke_checked([&]
    {
        auto state = state_of<tx_generated::semaphore_state>(value);
        std::lock_guard lock(state->mutex);
        if (permits <= 0 || permits > state->maximum - state->count)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "out_of_range", "信号量释放数量超出上限"});
        }
        state->count += permits;
        state->changed.notify_all();
    });
}

extern "C" int txrt_sync_new_once(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(
            std::make_shared<tx_generated::once_state>());
    });
}

extern "C" int txrt_sync_run_once(const void* value,
    const void* callback) noexcept
{
    return invoke_checked([&]
    {
        auto state = state_of<tx_generated::once_state>(value);
        {
            std::unique_lock lock(state->mutex);
            while (state->running)
            {
                if (state->owner == std::this_thread::get_id())
                {
                    throw tx_generated::runtime_failure({tx::error_kind::runtime,
                        "invalid_state", "once 回调不能递归调用同一个 once"});
                }
                state->changed.wait(lock);
            }
            if (state->done)
            {
                return;
            }
            state->running = true;
            state->owner = std::this_thread::get_id();
        }
        try
        {
            const auto& closure = std::any_cast<const tx_generated::closure_handle&>(
                *static_cast<const std::any*>(callback));
            reinterpret_cast<void (*)(const void*)>(
                closure.data().target)(callback);
        }
        catch (...)
        {
            {
                std::lock_guard lock(state->mutex);
                state->running = false;
                state->owner = {};
            }
            state->changed.notify_all();
            throw;
        }
        const bool succeeded = tx_generated::detail::current_runtime_context()
            .last_error_kind == tx::error_kind::none;
        {
            std::lock_guard lock(state->mutex);
            state->done = succeeded;
            state->running = false;
            state->owner = {};
        }
        state->changed.notify_all();
    });
}
