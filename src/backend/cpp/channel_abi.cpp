#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "backend/cpp/vector_abi_internal.hpp"
#include "stdlib/cancellation.hpp"
#include "stdlib/channel.hpp"

#include <algorithm>
#include <any>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{

using clock_type = std::chrono::steady_clock;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

struct select_signal
{
    std::mutex mutex;
    std::condition_variable changed;
    std::uint64_t epoch = 0;
};

select_signal& global_signal()
{
    // detach 线程在 main 返回期间仍可能通知，进程退出时不析构此同步状态。
    static auto* value = new select_signal();
    return *value;
}

void wake_selectors()
{
    auto& signal = global_signal();
    {
        std::lock_guard lock(signal.mutex);
        ++signal.epoch;
    }
    signal.changed.notify_all();
}

clock_type::time_point limit_from_ms(std::int64_t timeout_ms)
{
    if (timeout_ms < -1)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_argument", "通道超时只能为 -1 或非负毫秒"});
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
            "out_of_range", "通道超时超出单调时钟范围"});
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
            "cancelled", "通道操作已取消"});
    }
    if (token.state->deadline &&
        clock_type::now() >= *token.state->deadline)
    {
        throw tx_generated::runtime_failure({tx::error_kind::cancelled,
            "deadline_exceeded", "通道操作已超过截止时间"});
    }
}

clock_type::time_point next_wake(clock_type::time_point limit)
{
    return std::min(limit, clock_type::now() + std::chrono::milliseconds(10));
}

template<class value_type>
std::shared_ptr<tx_generated::channel_state<value_type>> channel_of(
    const void* value)
{
    return std::any_cast<const std::shared_ptr<
        tx_generated::channel_state<value_type>>&>(
            *static_cast<const std::any*>(value));
}

tx_generated::dynamic_struct option_value(const char* name, bool present,
                                           std::any value)
{
    tx_generated::struct_fields fields(2);
    fields[0] = {"present", present};
    fields[1] = {"value", present ? std::move(value) : std::any{}};
    return tx_generated::dynamic_struct(
        tx_generated::dynamic_struct_data{name, "option", std::move(fields)});
}

void* selected_value(const char* selected_name, const char* option_name,
    std::int64_t index, bool present, std::any value)
{
    tx_generated::struct_fields fields(2);
    fields[0] = {"index", index};
    fields[1] = {"value", option_value(option_name, present,
                                       std::move(value))};
    return make_handle<std::any>(tx_generated::dynamic_struct(
        tx_generated::dynamic_struct_data{
            selected_name, "selected", std::move(fields)}));
}

template<class value_type>
int bounded(std::int64_t capacity, void** result,
            std::string_view type_name = {}) noexcept
{
    return invoke_checked([&]
    {
        (void)global_signal();
        if (capacity <= 0 || capacity > 1'000'000)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "out_of_range", "通道容量必须在 1..1000000 内"});
        }
        auto state = std::make_shared<tx_generated::channel_state<value_type>>();
        state->capacity = static_cast<std::size_t>(capacity);
        state->type_name = type_name;
        *result = make_handle<std::any>(std::move(state));
    });
}

template<class value_type>
int send(const void* value, value_type item, std::int64_t timeout_ms,
         const void* token, bool* result) noexcept
{
    return invoke_checked([&]
    {
        auto state = channel_of<value_type>(value);
        const auto limit = limit_from_ms(timeout_ms);
        std::unique_lock lock(state->mutex);
        while (true)
        {
            if (state->closed)
            {
                throw tx_generated::runtime_failure({tx::error_kind::runtime,
                    "channel_closed", "不能向已关闭通道发送"});
            }
            if (state->queue.size() < state->capacity)
            {
                state->queue.push_back(item);
                lock.unlock();
                state->can_recv.notify_one();
                wake_selectors();
                *result = true;
                return;
            }
            check_cancel(token);
            if (clock_type::now() >= limit)
            {
                *result = false;
                return;
            }
            state->can_send.wait_until(lock, next_wake(limit));
        }
    });
}

template<class value_type, class receiver>
int recv_into(const void* value, std::int64_t timeout_ms,
              const void* token, receiver receive) noexcept
{
    return invoke_checked([&]
    {
        auto state = channel_of<value_type>(value);
        const auto limit = limit_from_ms(timeout_ms);
        std::unique_lock lock(state->mutex);
        while (true)
        {
            if (!state->queue.empty())
            {
                auto item = state->queue.front();
                receive(true, item);
                state->queue.pop_front();
                lock.unlock();
                state->can_send.notify_one();
                return;
            }
            if (state->closed)
            {
                receive(false, value_type{});
                return;
            }
            check_cancel(token);
            if (clock_type::now() >= limit)
            {
                throw tx_generated::runtime_failure({tx::error_kind::runtime,
                    "timeout", "通道接收超时"});
            }
            state->can_recv.wait_until(lock, next_wake(limit));
        }
    });
}

template<class value_type>
int recv(const void* value, std::int64_t timeout_ms,
         const void* token, const char* option_name, void** result) noexcept
{
    return recv_into<value_type>(value, timeout_ms, token,
        [&](bool present, const value_type& item)
        {
            *result = make_handle<std::any>(option_value(option_name, present, item));
        });
}

template<class value_type>
int recv_required(const void* value, std::int64_t timeout_ms,
                  const void* token, value_type* result) noexcept
{
    return recv_into<value_type>(value, timeout_ms, token,
        [&](bool present, value_type item)
        {
            if (!present)
            {
                throw tx_generated::runtime_failure({tx::error_kind::runtime,
                    "invalid_state", "空 option 没有值"});
            }
            *result = item;
        });
}

template<class value_type>
int close_channel(const void* value, bool* result) noexcept
{
    return invoke_checked([&]
    {
        auto state = channel_of<value_type>(value);
        {
            std::lock_guard lock(state->mutex);
            *result = !state->closed;
            state->closed = true;
        }
        state->can_send.notify_all();
        state->can_recv.notify_all();
        wake_selectors();
    });
}

template<class value_type>
int select_channels(const void* values, std::int64_t timeout_ms,
    const void* token, const char* selected_name,
    const char* option_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& channels = tx_generated::detail::vector_value<std::any>(
            values).data().values;
        if (channels.empty())
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "invalid_argument", "select 至少需要一个通道"});
        }
        const auto limit = limit_from_ms(timeout_ms);
        static thread_local std::size_t next_index = 0;
        std::int64_t selected = -1;
        while (true)
        {
            auto& signal = global_signal();
            std::uint64_t before;
            {
                std::lock_guard lock(signal.mutex);
                before = signal.epoch;
            }
            for (std::size_t offset = 0; offset < channels.size(); ++offset)
            {
                const auto index = (next_index + offset) % channels.size();
                auto state = channel_of<value_type>(&channels[index]);
                std::unique_lock lock(state->mutex);
                if (!state->queue.empty())
                {
                    auto* output = selected_value(selected_name, option_name,
                        static_cast<std::int64_t>(index), true,
                        state->queue.front());
                    state->queue.pop_front();
                    selected = static_cast<std::int64_t>(index);
                    lock.unlock();
                    state->can_send.notify_one();
                    *result = output;
                    break;
                }
                if (state->closed)
                {
                    selected = static_cast<std::int64_t>(index);
                    *result = selected_value(selected_name, option_name,
                        selected, false, {});
                    break;
                }
            }
            if (selected >= 0)
            {
                next_index = (static_cast<std::size_t>(selected) + 1) %
                    channels.size();
                break;
            }
            check_cancel(token);
            if (clock_type::now() >= limit)
            {
                break;
            }
            std::unique_lock lock(signal.mutex);
            if (signal.epoch == before)
            {
                signal.changed.wait_until(lock, next_wake(limit));
            }
        }
        if (selected < 0)
        {
            *result = selected_value(selected_name, option_name,
                -1, false, {});
        }
    });
}

} // namespace

#define TX_CHANNEL_ABI(SUFFIX, TYPE) \
extern "C" int txrt_channel_recv_required_##SUFFIX(const void* channel, \
    std::int64_t timeout_ms, const void* token, TYPE* result) noexcept \
{ \
    return recv_required<TYPE>(channel, timeout_ms, token, result); \
} \
extern "C" int txrt_channel_bounded_##SUFFIX(std::int64_t capacity, \
    void** result) noexcept \
{ \
    return bounded<TYPE>(capacity, result); \
} \
extern "C" int txrt_channel_send_##SUFFIX(const void* channel, TYPE item, \
    std::int64_t timeout_ms, const void* token, bool* result) noexcept \
{ \
    return send(channel, item, timeout_ms, token, result); \
} \
extern "C" int txrt_channel_recv_##SUFFIX(const void* channel, \
    std::int64_t timeout_ms, const void* token, const char* option_name, \
    void** result) noexcept \
{ \
    return recv<TYPE>(channel, timeout_ms, token, option_name, result); \
} \
extern "C" int txrt_channel_close_##SUFFIX(const void* channel, \
    bool* result) noexcept \
{ \
    return close_channel<TYPE>(channel, result); \
} \
extern "C" int txrt_channel_select_##SUFFIX(const void* channels, \
    std::int64_t timeout_ms, const void* token, const char* selected_name, \
    const char* option_name, void** result) noexcept \
{ \
    return select_channels<TYPE>(channels, timeout_ms, token, \
        selected_name, option_name, result); \
}

TX_CHANNEL_ABI(i64, std::int64_t)
TX_CHANNEL_ABI(f64, double)
TX_CHANNEL_ABI(bool, bool)

#undef TX_CHANNEL_ABI

#define TX_CHANNEL_HANDLE_ABI(SUFFIX, TYPE) \
extern "C" int txrt_channel_bounded_##SUFFIX(std::int64_t capacity, \
    const char* type_name, void** result) noexcept \
{ \
    return bounded<TYPE>(capacity, result, type_name); \
} \
extern "C" int txrt_channel_send_##SUFFIX(const void* channel, \
    const void* item, std::int64_t timeout_ms, const void* token, \
    bool* result) noexcept \
{ \
    if (!item) \
    { \
        return invoke_checked([&] \
        { \
            throw tx_generated::runtime_failure({tx::error_kind::runtime, \
                "invalid_argument", "通道消息为空"}); \
        }); \
    } \
    return send<TYPE>(channel, *static_cast<const TYPE*>(item), \
        timeout_ms, token, result); \
} \
extern "C" int txrt_channel_recv_##SUFFIX(const void* channel, \
    std::int64_t timeout_ms, const void* token, const char* option_name, \
    void** result) noexcept \
{ \
    return recv<TYPE>(channel, timeout_ms, token, option_name, result); \
} \
extern "C" int txrt_channel_close_##SUFFIX(const void* channel, \
    bool* result) noexcept \
{ \
    return close_channel<TYPE>(channel, result); \
} \
extern "C" int txrt_channel_select_##SUFFIX(const void* channels, \
    std::int64_t timeout_ms, const void* token, const char* selected_name, \
    const char* option_name, void** result) noexcept \
{ \
    return select_channels<TYPE>(channels, timeout_ms, token, \
        selected_name, option_name, result); \
}

TX_CHANNEL_HANDLE_ABI(str, std::string)
TX_CHANNEL_HANDLE_ABI(value, std::any)

#undef TX_CHANNEL_HANDLE_ABI
