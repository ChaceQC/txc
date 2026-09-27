#include "backend/cpp/cancel_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/cancellation.hpp"

#include <algorithm>
#include <any>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>

namespace
{

using clock_type = std::chrono::steady_clock;
using state_type = tx_generated::cancellation_state;

std::shared_ptr<state_type> source_state(const void* value)
{
    const auto& owner = std::any_cast<const tx_generated::cancel_source&>(
        *static_cast<const std::any*>(value));
    if (!owner.state)
    {
        throw std::runtime_error("取消源已失效");
    }
    return owner.state;
}

const std::shared_ptr<state_type>& token_state(const void* value)
{
    const auto& token = std::any_cast<const tx_generated::cancel_token&>(
        *static_cast<const std::any*>(value));
    if (!token.state)
    {
        throw std::runtime_error("取消令牌已失效");
    }
    return token.state;
}

clock_type::time_point deadline_from_delay(std::int64_t delay_ms)
{
    if (delay_ms < 0)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_argument", "截止时间不能为负"});
    }
    const auto now = clock_type::now();
    const auto available = std::chrono::duration_cast<std::chrono::milliseconds>(
        clock_type::time_point::max() - now).count();
    if (delay_ms > available)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "out_of_range", "截止时间超出单调时钟范围"});
    }
    return now + std::chrono::milliseconds(delay_ms);
}

std::int64_t current_status(const state_type& state)
{
    // 同一临界区内主动取消先于截止到期，状态不回退。
    if (state.cancelled)
    {
        return 1;
    }
    if (state.deadline && clock_type::now() >= *state.deadline)
    {
        return 2;
    }
    return 0;
}

std::int64_t wait_for_status(state_type& state, std::int64_t timeout_ms)
{
    if (timeout_ms < -1)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_argument", "等待超时只能为 -1 或非负毫秒"});
    }
    const auto limit = timeout_ms == -1
        ? clock_type::time_point::max()
        : deadline_from_delay(timeout_ms);
    std::unique_lock lock(state.mutex);
    while (true)
    {
        if (const auto status = current_status(state); status != 0)
        {
            return status;
        }
        if (clock_type::now() >= limit)
        {
            return 0;
        }
        const auto wake_at = std::min(limit,
            state.deadline.value_or(clock_type::time_point::max()));
        if (wake_at == clock_type::time_point::max())
        {
            state.changed.wait(lock);
        }
        else
        {
            state.changed.wait_until(lock, wake_at);
        }
    }
}

} // namespace

extern "C" int txrt_cancel_source(void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto state = std::make_shared<state_type>();
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::cancel_source{std::move(state)});
    });
}

extern "C" int txrt_cancel_with_deadline_ms(std::int64_t timeout_ms,
    void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto state = std::make_shared<state_type>();
        state->deadline = deadline_from_delay(timeout_ms);
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::cancel_source{std::move(state)});
    });
}

extern "C" int txrt_cancel_token(const void* owner, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::cancel_token{source_state(owner)});
    });
}

extern "C" int txrt_cancel_cancel(const void* owner, bool* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto state = source_state(owner);
        {
            std::lock_guard lock(state->mutex);
            *result = current_status(*state) == 0;
            if (*result)
            {
                state->cancelled = true;
            }
        }
        if (*result)
        {
            state->changed.notify_all();
        }
    });
}

extern "C" int txrt_cancel_status(const void* value,
    std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        const auto& state = token_state(value);
        std::lock_guard lock(state->mutex);
        *result = current_status(*state);
    });
}

extern "C" int txrt_cancel_wait(const void* value,
    std::int64_t timeout_ms, std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        // 阻塞等待继续持有独立引用；只有同步状态查询省略这次持有。
        const auto state = token_state(value);
        *result = wait_for_status(*state, timeout_ms);
    });
}
