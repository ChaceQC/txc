#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/cancellation.hpp"

#include <algorithm>
#include <any>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace
{

using clock_type = std::chrono::steady_clock;
using tx_generated::dynamic_struct;
using tx_generated::dynamic_struct_data;
using tx_generated::struct_fields;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

[[noreturn]] void fail(const char* code, const char* message)
{
    throw tx_generated::runtime_failure({tx::error_kind::runtime, code, message});
}

std::int64_t checked_add(std::int64_t left, std::int64_t right)
{
    if ((right > 0 && left > std::numeric_limits<std::int64_t>::max() - right) ||
        (right < 0 && left < std::numeric_limits<std::int64_t>::min() - right))
    {
        fail("out_of_range", "时间值加法溢出");
    }
    return left + right;
}

std::int64_t checked_sub(std::int64_t left, std::int64_t right)
{
    if ((right < 0 && left > std::numeric_limits<std::int64_t>::max() + right) ||
        (right > 0 && left < std::numeric_limits<std::int64_t>::min() + right))
    {
        fail("out_of_range", "时间值减法溢出");
    }
    return left - right;
}

std::int64_t checked_scale(std::int64_t value, std::int64_t scale)
{
    if (value > std::numeric_limits<std::int64_t>::max() / scale ||
        value < std::numeric_limits<std::int64_t>::min() / scale)
    {
        fail("out_of_range", "时间单位转换溢出");
    }
    return value * scale;
}

std::int64_t field(const void* value, const char* expected)
{
    const auto& object = std::any_cast<const dynamic_struct&>(
        *static_cast<const std::any*>(value));
    if (object->display_name != expected || object->fields.size() != 1)
    {
        fail("invalid_argument", "时间值结构不匹配");
    }
    return std::any_cast<std::int64_t>(object->fields[0].value);
}

void* make_value(const char* type_name, const char* display_name,
                 const char* field_name, std::int64_t value)
{
    struct_fields fields(1);
    fields[0] = {field_name, value};
    return make_handle<std::any>(dynamic_struct(dynamic_struct_data{
        type_name, display_name, std::move(fields)}));
}

void* make_duration(const char* type_name, std::int64_t value)
{
    return make_value(type_name, "duration", "micros", value);
}

void* make_instant(const char* type_name, std::int64_t value)
{
    return make_value(type_name, "instant", "ticks_micros", value);
}

void require_nonnegative(std::int64_t micros)
{
    if (micros < 0)
    {
        fail("invalid_argument", "睡眠时长不能为负");
    }
}

void check_cancellation(const tx_generated::cancellation_state& state)
{
    // 取消与截止时间共用单调时钟；主动取消在同一临界区内优先。
    if (state.cancelled)
    {
        throw tx_generated::runtime_failure({tx::error_kind::cancelled,
            "cancelled", "睡眠已取消"});
    }
    if (state.deadline && clock_type::now() >= *state.deadline)
    {
        throw tx_generated::runtime_failure({tx::error_kind::cancelled,
            "deadline_exceeded", "睡眠截止时间已到"});
    }
}

void sleep_for(std::int64_t micros,
               const std::shared_ptr<tx_generated::cancellation_state>& state)
{
    require_nonnegative(micros);
    constexpr std::int64_t day_micros = 24LL * 60 * 60 * 1000000;
    while (true)
    {
        if (state)
        {
            std::unique_lock lock(state->mutex);
            check_cancellation(*state);
            if (micros == 0)
            {
                return;
            }
            const auto chunk = std::min(micros, day_micros);
            const auto end = clock_type::now() + std::chrono::microseconds(chunk);
            while (clock_type::now() < end)
            {
                check_cancellation(*state);
                state->changed.wait_until(lock,
                    std::min(end, state->deadline.value_or(end)));
            }
            micros -= chunk;
        }
        else
        {
            if (micros == 0)
            {
                return;
            }
            const auto chunk = std::min(micros, day_micros);
            std::this_thread::sleep_for(std::chrono::microseconds(chunk));
            micros -= chunk;
        }
    }
}

} // namespace

extern "C" int txrt_time_duration_from_micros(std::int64_t micros,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&] { *result = make_duration(type_name, micros); });
}

extern "C" int txrt_time_duration_from_millis(std::int64_t millis,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_duration(type_name, checked_scale(millis, 1000));
    });
}

extern "C" int txrt_time_duration_from_seconds(std::int64_t seconds,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_duration(type_name, checked_scale(seconds, 1000000));
    });
}

extern "C" int txrt_time_duration_add(const void* left, const void* right,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_duration(type_name,
            checked_add(field(left, "duration"), field(right, "duration")));
    });
}

extern "C" int txrt_time_duration_sub(const void* left, const void* right,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_duration(type_name,
            checked_sub(field(left, "duration"), field(right, "duration")));
    });
}

extern "C" int txrt_time_duration_negate(const void* value,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_duration(type_name, checked_sub(0, field(value, "duration")));
    });
}

extern "C" int txrt_time_duration_as_micros(const void* value,
    std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = field(value, "duration"); });
}

extern "C" int txrt_time_duration_as_millis(const void* value,
    std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = field(value, "duration") / 1000; });
}

extern "C" int txrt_time_instant_now(const char* type_name,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto ticks = std::chrono::duration_cast<std::chrono::microseconds>(
            clock_type::now().time_since_epoch()).count();
        *result = make_instant(type_name, ticks);
    });
}

extern "C" int txrt_time_instant_elapsed(const void* start,
    const void* finish, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_duration(type_name,
            checked_sub(field(finish, "instant"), field(start, "instant")));
    });
}

extern "C" int txrt_time_instant_after(const void* start,
    const void* delay, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_instant(type_name,
            checked_add(field(start, "instant"), field(delay, "duration")));
    });
}

extern "C" int txrt_time_sleep(const void* value) noexcept
{
    return invoke_checked([&] { sleep_for(field(value, "duration"), {}); });
}

extern "C" int txrt_time_sleep_cancelled(const void* value,
    const void* token) noexcept
{
    return invoke_checked([&]
    {
        const auto& cancellation = std::any_cast<const tx_generated::cancel_token&>(
            *static_cast<const std::any*>(token));
        if (!cancellation.state)
        {
            fail("invalid_argument", "取消令牌无效");
        }
        sleep_for(field(value, "duration"), cancellation.state);
    });
}
