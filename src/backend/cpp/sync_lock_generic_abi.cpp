#include "backend/cpp/deep_copy.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/synchronization.hpp"

#include <any>
#include <memory>
#include <string>

namespace
{

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

template<class value_type>
value_type detached_copy(const value_type& value)
{
    if constexpr (std::is_same_v<value_type, std::any>)
    {
        return tx_generated::deep_copy_value(value);
    }
    return value;
}

template<class state_type>
std::shared_ptr<state_type> state_of(const void* value)
{
    return std::any_cast<const std::shared_ptr<state_type>&>(
        *static_cast<const std::any*>(value));
}

template<class guard_type>
void require_locked(const std::shared_ptr<guard_type>& guard)
{
    if (!guard->lock.owns_lock())
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_state", "锁卫士已关闭"});
    }
}

template<class value_type>
int new_mutex(const void* initial, const char* type_name,
              void** result) noexcept
{
    return invoke_checked([&]
    {
        auto owner = std::make_shared<tx_generated::mutex_state<value_type>>();
        owner->value = *static_cast<const value_type*>(initial);
        owner->type_name = type_name;
        *result = make_handle<std::any>(std::move(owner));
    });
}

template<class value_type>
int lock_mutex(const void* owner, void** result) noexcept
{
    return invoke_checked([&]
    {
        using state_type = tx_generated::mutex_state<value_type>;
        auto guard = std::make_shared<
            tx_generated::mutex_guard_state<value_type>>(
                state_of<state_type>(owner));
        *result = make_handle<std::any>(std::move(guard));
    });
}

template<class value_type>
int get_mutex(const void* guard, void** result) noexcept
{
    return invoke_checked([&]
    {
        using guard_type = tx_generated::mutex_guard_state<value_type>;
        auto held = state_of<guard_type>(guard);
        require_locked(held);
        *result = make_handle<value_type>(
            detached_copy(held->owner->value));
    });
}

template<class value_type>
int set_mutex(const void* guard, const void* value) noexcept
{
    return invoke_checked([&]
    {
        using guard_type = tx_generated::mutex_guard_state<value_type>;
        auto held = state_of<guard_type>(guard);
        require_locked(held);
        held->owner->value = *static_cast<const value_type*>(value);
    });
}

template<class value_type>
int close_mutex(const void* guard) noexcept
{
    return invoke_checked([&]
    {
        using guard_type = tx_generated::mutex_guard_state<value_type>;
        auto held = state_of<guard_type>(guard);
        require_locked(held);
        held->lock.unlock();
    });
}

template<class value_type>
int new_rw_lock(const void* initial, const char* type_name,
                void** result) noexcept
{
    return invoke_checked([&]
    {
        auto owner = std::make_shared<tx_generated::rw_lock_state<value_type>>();
        owner->value = *static_cast<const value_type*>(initial);
        owner->type_name = type_name;
        *result = make_handle<std::any>(std::move(owner));
    });
}

template<class value_type, bool write>
int lock_rw(const void* owner, void** result) noexcept
{
    return invoke_checked([&]
    {
        using state_type = tx_generated::rw_lock_state<value_type>;
        auto guard = std::make_shared<
            tx_generated::rw_guard_state<value_type, write>>(
                state_of<state_type>(owner));
        *result = make_handle<std::any>(std::move(guard));
    });
}

template<class value_type, bool write>
int get_rw(const void* guard, void** result) noexcept
{
    return invoke_checked([&]
    {
        using guard_type = tx_generated::rw_guard_state<value_type, write>;
        auto held = state_of<guard_type>(guard);
        require_locked(held);
        *result = make_handle<value_type>(
            detached_copy(held->owner->value));
    });
}

template<class value_type>
int set_rw(const void* guard, const void* value) noexcept
{
    return invoke_checked([&]
    {
        using guard_type = tx_generated::rw_guard_state<value_type, true>;
        auto held = state_of<guard_type>(guard);
        require_locked(held);
        held->owner->value = *static_cast<const value_type*>(value);
    });
}

template<class value_type, bool write>
int close_rw(const void* guard) noexcept
{
    return invoke_checked([&]
    {
        using guard_type = tx_generated::rw_guard_state<value_type, write>;
        auto held = state_of<guard_type>(guard);
        require_locked(held);
        held->lock.unlock();
    });
}

} // namespace

#define TX_SYNC_GENERIC_ABI(SUFFIX, TYPE) \
extern "C" int txrt_sync_new_mutex_##SUFFIX(const void* value, \
    const char* type_name, void** output) noexcept \
{ \
    return new_mutex<TYPE>(value, type_name, output); \
} \
extern "C" int txrt_sync_lock_##SUFFIX(const void* value, \
    void** output) noexcept \
{ \
    return lock_mutex<TYPE>(value, output); \
} \
extern "C" int txrt_sync_guard_get_##SUFFIX(const void* value, \
    void** output) noexcept \
{ \
    return get_mutex<TYPE>(value, output); \
} \
extern "C" int txrt_sync_guard_set_##SUFFIX(const void* value, \
    const void* next) noexcept \
{ \
    return set_mutex<TYPE>(value, next); \
} \
extern "C" int txrt_sync_guard_close_##SUFFIX(const void* value) noexcept \
{ \
    return close_mutex<TYPE>(value); \
} \
extern "C" int txrt_sync_new_rw_lock_##SUFFIX(const void* value, \
    const char* type_name, void** output) noexcept \
{ \
    return new_rw_lock<TYPE>(value, type_name, output); \
} \
extern "C" int txrt_sync_read_lock_##SUFFIX(const void* value, \
    void** output) noexcept \
{ \
    return lock_rw<TYPE, false>(value, output); \
} \
extern "C" int txrt_sync_write_lock_##SUFFIX(const void* value, \
    void** output) noexcept \
{ \
    return lock_rw<TYPE, true>(value, output); \
} \
extern "C" int txrt_sync_read_get_##SUFFIX(const void* value, \
    void** output) noexcept \
{ \
    return get_rw<TYPE, false>(value, output); \
} \
extern "C" int txrt_sync_write_get_##SUFFIX(const void* value, \
    void** output) noexcept \
{ \
    return get_rw<TYPE, true>(value, output); \
} \
extern "C" int txrt_sync_write_set_##SUFFIX(const void* value, \
    const void* next) noexcept \
{ \
    return set_rw<TYPE>(value, next); \
} \
extern "C" int txrt_sync_read_close_##SUFFIX(const void* value) noexcept \
{ \
    return close_rw<TYPE, false>(value); \
} \
extern "C" int txrt_sync_write_close_##SUFFIX(const void* value) noexcept \
{ \
    return close_rw<TYPE, true>(value); \
}

TX_SYNC_GENERIC_ABI(str, std::string)
TX_SYNC_GENERIC_ABI(value, std::any)

#undef TX_SYNC_GENERIC_ABI
