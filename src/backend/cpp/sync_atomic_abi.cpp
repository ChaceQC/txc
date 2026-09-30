#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/synchronization.hpp"

#include <any>
#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>

namespace
{

using tx_generated::detail::invoke_leaf;
using tx_generated::detail::make_handle;

std::memory_order parse_order(std::int64_t value)
{
    switch (value)
    {
    case 0: return std::memory_order_relaxed;
    case 1: return std::memory_order_acquire;
    case 2: return std::memory_order_release;
    case 3: return std::memory_order_acq_rel;
    case 4: return std::memory_order_seq_cst;
    default:
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_argument", "原子内存序必须是 0..4"});
    }
}

std::memory_order load_order(std::int64_t value)
{
    const auto order = parse_order(value);
    if (order == std::memory_order_release ||
        order == std::memory_order_acq_rel)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_argument", "原子 load 不能使用 release/acq_rel"});
    }
    return order;
}

std::memory_order store_order(std::int64_t value)
{
    const auto order = parse_order(value);
    if (order == std::memory_order_acquire ||
        order == std::memory_order_acq_rel)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_argument", "原子 store 不能使用 acquire/acq_rel"});
    }
    return order;
}

template<class value_type>
const std::shared_ptr<std::atomic<value_type>>& atomic_of(const void* value)
{
    return std::any_cast<const std::shared_ptr<std::atomic<value_type>>&>(
        *static_cast<const std::any*>(value));
}

template<class value_type>
int new_atomic(value_type initial, void** result) noexcept
{
    return invoke_leaf([&]
    {
        *result = make_handle<std::any>(
            std::make_shared<std::atomic<value_type>>(initial));
    });
}

template<class value_type>
int atomic_load(const void* value, std::int64_t order,
                value_type* result) noexcept
{
    return invoke_leaf([&]
    {
        *result = atomic_of<value_type>(value)->load(load_order(order));
    });
}

template<class value_type>
int atomic_store(const void* value, value_type next,
                 std::int64_t order) noexcept
{
    return invoke_leaf([&]
    {
        atomic_of<value_type>(value)->store(next, store_order(order));
    });
}

template<class value_type>
int atomic_exchange(const void* value, value_type next,
                    std::int64_t order, value_type* result) noexcept
{
    return invoke_leaf([&]
    {
        *result = atomic_of<value_type>(value)->exchange(
            next, parse_order(order));
    });
}

template<class value_type>
int atomic_compare_exchange(const void* value, value_type expected,
    value_type desired, std::int64_t order, bool* result) noexcept
{
    return invoke_leaf([&]
    {
        // 失败读取使用 relaxed，成功顺序由调用方明确选择。
        *result = atomic_of<value_type>(value)->compare_exchange_strong(
            expected, desired, parse_order(order),
            std::memory_order_relaxed);
    });
}

} // namespace

extern "C" int txrt_sync_new_atomic_i64(std::int64_t initial,
    void** result) noexcept
{
    return new_atomic(initial, result);
}

extern "C" int txrt_sync_new_atomic_bool(bool initial,
    void** result) noexcept
{
    return new_atomic(initial, result);
}

extern "C" int txrt_sync_atomic_load_i64(const void* value,
    std::int64_t order, std::int64_t* result) noexcept
{
    return atomic_load(value, order, result);
}

extern "C" int txrt_sync_atomic_load_bool(const void* value,
    std::int64_t order, bool* result) noexcept
{
    return atomic_load(value, order, result);
}

extern "C" int txrt_sync_atomic_store_i64(const void* value,
    std::int64_t next, std::int64_t order) noexcept
{
    return atomic_store(value, next, order);
}

extern "C" int txrt_sync_atomic_store_bool(const void* value,
    bool next, std::int64_t order) noexcept
{
    return atomic_store(value, next, order);
}

extern "C" int txrt_sync_atomic_exchange_i64(const void* value,
    std::int64_t next, std::int64_t order,
    std::int64_t* result) noexcept
{
    return atomic_exchange(value, next, order, result);
}

extern "C" int txrt_sync_atomic_exchange_bool(const void* value,
    bool next, std::int64_t order, bool* result) noexcept
{
    return atomic_exchange(value, next, order, result);
}

extern "C" int txrt_sync_atomic_compare_exchange_i64(const void* value,
    std::int64_t expected, std::int64_t desired, std::int64_t order,
    bool* result) noexcept
{
    return atomic_compare_exchange(value, expected, desired, order, result);
}

extern "C" int txrt_sync_atomic_compare_exchange_bool(const void* value,
    bool expected, bool desired, std::int64_t order,
    bool* result) noexcept
{
    return atomic_compare_exchange(value, expected, desired, order, result);
}

extern "C" int txrt_sync_atomic_fetch_add_i64(const void* value,
    std::int64_t delta, std::int64_t order, std::int64_t* result) noexcept
{
    return invoke_leaf([&]
    {
        const auto& owner = atomic_of<std::int64_t>(value);
        const auto memory_order = parse_order(order);
        auto current = owner->load(std::memory_order_relaxed);
        while (true)
        {
            if ((delta > 0 && current >
                    std::numeric_limits<std::int64_t>::max() - delta) ||
                (delta < 0 && current <
                    std::numeric_limits<std::int64_t>::min() - delta))
            {
                throw tx_generated::runtime_failure({tx::error_kind::runtime,
                    "overflow", "原子加法溢出"});
            }
            if (owner->compare_exchange_weak(current, current + delta,
                    memory_order, std::memory_order_relaxed))
            {
                *result = current;
                return;
            }
        }
    });
}
