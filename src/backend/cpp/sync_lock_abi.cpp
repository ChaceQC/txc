#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/synchronization.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace
{

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

template<class value_type>
std::shared_ptr<tx_generated::mutex_state<value_type>> mutex_of(
    const void* value)
{
    return std::any_cast<const std::shared_ptr<
        tx_generated::mutex_state<value_type>>&>(
            *static_cast<const std::any*>(value));
}

template<class value_type>
std::shared_ptr<tx_generated::mutex_guard_state<value_type>> guard_of(
    const void* value)
{
    return std::any_cast<const std::shared_ptr<
        tx_generated::mutex_guard_state<value_type>>&>(
            *static_cast<const std::any*>(value));
}

template<class value_type>
std::shared_ptr<tx_generated::rw_lock_state<value_type>> rw_of(
    const void* value)
{
    return std::any_cast<const std::shared_ptr<
        tx_generated::rw_lock_state<value_type>>&>(
            *static_cast<const std::any*>(value));
}

template<class value_type, bool write>
std::shared_ptr<tx_generated::rw_guard_state<value_type, write>> rw_guard_of(
    const void* value)
{
    return std::any_cast<const std::shared_ptr<
        tx_generated::rw_guard_state<value_type, write>>&>(
            *static_cast<const std::any*>(value));
}

template<class guard_type>
void require_locked(const guard_type& guard)
{
    if (!guard->lock.owns_lock())
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_state", "锁卫士已关闭"});
    }
}

template<class value_type>
int new_mutex(value_type initial, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto owner = std::make_shared<tx_generated::mutex_state<value_type>>();
        owner->value = initial;
        *result = make_handle<std::any>(std::move(owner));
    });
}

template<class value_type>
int lock_mutex(const void* owner, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto guard = std::make_shared<tx_generated::mutex_guard_state<value_type>>(
            mutex_of<value_type>(owner));
        *result = make_handle<std::any>(std::move(guard));
    });
}

template<class value_type>
int get_mutex(const void* guard, value_type* result) noexcept
{
    return invoke_checked([&]
    {
        auto state = guard_of<value_type>(guard);
        require_locked(state);
        *result = state->owner->value;
    });
}

template<class value_type>
int set_mutex(const void* guard, value_type value) noexcept
{
    return invoke_checked([&]
    {
        auto state = guard_of<value_type>(guard);
        require_locked(state);
        state->owner->value = value;
    });
}

template<class value_type>
int close_mutex(const void* guard) noexcept
{
    return invoke_checked([&]
    {
        auto state = guard_of<value_type>(guard);
        require_locked(state);
        state->lock.unlock();
    });
}

template<class value_type>
int new_rw_lock(value_type initial, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto owner = std::make_shared<tx_generated::rw_lock_state<value_type>>();
        owner->value = initial;
        *result = make_handle<std::any>(std::move(owner));
    });
}

template<class value_type, bool write>
int lock_rw(const void* owner, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto guard = std::make_shared<
            tx_generated::rw_guard_state<value_type, write>>(
                rw_of<value_type>(owner));
        *result = make_handle<std::any>(std::move(guard));
    });
}

template<class value_type, bool write>
int get_rw(const void* guard, value_type* result) noexcept
{
    return invoke_checked([&]
    {
        auto state = rw_guard_of<value_type, write>(guard);
        require_locked(state);
        *result = state->owner->value;
    });
}

template<class value_type>
int set_rw(const void* guard, value_type value) noexcept
{
    return invoke_checked([&]
    {
        auto state = rw_guard_of<value_type, true>(guard);
        require_locked(state);
        state->owner->value = value;
    });
}

template<class value_type, bool write>
int close_rw(const void* guard) noexcept
{
    return invoke_checked([&]
    {
        auto state = rw_guard_of<value_type, write>(guard);
        require_locked(state);
        state->lock.unlock();
    });
}

} // namespace

#define TX_SYNC_LOCK_ABI(SUFFIX, TYPE) \
extern "C" int txrt_sync_new_mutex_##SUFFIX(TYPE value, void** output) noexcept \
{ \
    return new_mutex(value, output); \
} \
extern "C" int txrt_sync_lock_##SUFFIX(const void* value, void** output) noexcept \
{ \
    return lock_mutex<TYPE>(value, output); \
} \
extern "C" int txrt_sync_guard_get_##SUFFIX(const void* value, TYPE* output) noexcept \
{ \
    return get_mutex(value, output); \
} \
extern "C" int txrt_sync_guard_set_##SUFFIX(const void* value, TYPE next) noexcept \
{ \
    return set_mutex(value, next); \
} \
extern "C" int txrt_sync_guard_close_##SUFFIX(const void* value) noexcept \
{ \
    return close_mutex<TYPE>(value); \
} \
extern "C" int txrt_sync_new_rw_lock_##SUFFIX(TYPE value, void** output) noexcept \
{ \
    return new_rw_lock(value, output); \
} \
extern "C" int txrt_sync_read_lock_##SUFFIX(const void* value, void** output) noexcept \
{ \
    return lock_rw<TYPE, false>(value, output); \
} \
extern "C" int txrt_sync_write_lock_##SUFFIX(const void* value, void** output) noexcept \
{ \
    return lock_rw<TYPE, true>(value, output); \
} \
extern "C" int txrt_sync_read_get_##SUFFIX(const void* value, TYPE* output) noexcept \
{ \
    return get_rw<TYPE, false>(value, output); \
} \
extern "C" int txrt_sync_write_get_##SUFFIX(const void* value, TYPE* output) noexcept \
{ \
    return get_rw<TYPE, true>(value, output); \
} \
extern "C" int txrt_sync_write_set_##SUFFIX(const void* value, TYPE next) noexcept \
{ \
    return set_rw(value, next); \
} \
extern "C" int txrt_sync_read_close_##SUFFIX(const void* value) noexcept \
{ \
    return close_rw<TYPE, false>(value); \
} \
extern "C" int txrt_sync_write_close_##SUFFIX(const void* value) noexcept \
{ \
    return close_rw<TYPE, true>(value); \
}

TX_SYNC_LOCK_ABI(i64, std::int64_t)
TX_SYNC_LOCK_ABI(f64, double)
TX_SYNC_LOCK_ABI(bool, bool)

#undef TX_SYNC_LOCK_ABI
