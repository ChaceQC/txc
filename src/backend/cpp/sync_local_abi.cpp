#include "backend/cpp/runtime_abi_internal.hpp"
#include "common/local_resource_layout.hpp"
#include "stdlib/synchronization.hpp"

#include <cstddef>
#include <memory>
#include <new>

namespace
{

struct local_guard_storage
{
    bool active;
    alignas(tx::local_guard_alignment) std::byte data[64];
};

static_assert(sizeof(local_guard_storage) == tx::local_guard_bytes);

template<class value_type>
using guard_type = tx_generated::mutex_guard_state<value_type>;

template<class value_type>
guard_type<value_type>& state(void* storage)
{
    auto& local = *static_cast<local_guard_storage*>(storage);
    if (!local.active)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_state", "锁卫士已关闭"});
    }
    auto& guard = *std::launder(reinterpret_cast<guard_type<value_type>*>(local.data));
    if (!guard.lock.owns_lock())
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_state", "锁卫士已关闭"});
    }
    return guard;
}

template<class value_type>
int acquire(const void* owner, void* storage) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        static_assert(sizeof(guard_type<value_type>) <= sizeof(local_guard_storage::data));
        static_assert(alignof(guard_type<value_type>) <= tx::local_guard_alignment);
        auto& local = *static_cast<local_guard_storage*>(storage);
        const auto& mutex = std::any_cast<const std::shared_ptr<tx_generated::mutex_state<value_type>>&>(
            *static_cast<const std::any*>(owner));
        // 锁成功后才发布 active；构造失败时 shared_ptr 成员自动撤销。
        std::construct_at(reinterpret_cast<guard_type<value_type>*>(local.data), mutex);
        local.active = true;
    });
}

template<class value_type>
void destroy(void* storage) noexcept
{
    auto& local = *static_cast<local_guard_storage*>(storage);
    if (local.active)
    {
        local.active = false;
        std::destroy_at(std::launder(reinterpret_cast<guard_type<value_type>*>(local.data)));
    }
}

} // namespace

#define TX_LOCAL_GUARD(SUFFIX, TYPE) \
extern "C" int txrt_sync_local_lock_##SUFFIX(const void* owner, void* output) noexcept \
{ \
    return acquire<TYPE>(owner, output); \
} \
extern "C" int txrt_sync_local_get_##SUFFIX(void* guard, TYPE* output) noexcept \
{ \
    return tx_generated::detail::invoke_leaf([&] \
    { \
        *output = state<TYPE>(guard).owner->value; \
    }); \
} \
extern "C" int txrt_sync_local_set_##SUFFIX(void* guard, TYPE value) noexcept \
{ \
    return tx_generated::detail::invoke_leaf([&] \
    { \
        state<TYPE>(guard).owner->value = value; \
    }); \
} \
extern "C" int txrt_sync_local_close_##SUFFIX(void* guard) noexcept \
{ \
    return tx_generated::detail::invoke_leaf([&] \
    { \
        state<TYPE>(guard).lock.unlock(); \
    }); \
} \
extern "C" void txrt_sync_local_destroy_##SUFFIX(void* guard) noexcept \
{ \
    destroy<TYPE>(guard); \
}

TX_LOCAL_GUARD(i64, std::int64_t)
TX_LOCAL_GUARD(f64, double)
TX_LOCAL_GUARD(bool, bool)

#undef TX_LOCAL_GUARD
