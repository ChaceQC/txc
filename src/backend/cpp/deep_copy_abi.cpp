#include "backend/cpp/deep_copy.hpp"
#include "backend/cpp/value_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"

extern "C" int txrt_value_deep_copy(const void* value, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(tx_generated::deep_copy_value(
            *static_cast<const std::any*>(value)));
    });
}

extern "C" int txrt_value_deep_copy_known(const void* value, std::uint64_t kind, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(tx_generated::deep_copy_known(
            *static_cast<const std::any*>(value), static_cast<tx::record_copy_kind>(kind)));
    });
}
