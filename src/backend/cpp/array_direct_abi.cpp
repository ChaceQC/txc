#include "backend/cpp/external_direct_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <any>

namespace
{

const tx_generated::tx_array& array_value(const void* value)
{
    return std::any_cast<const tx_generated::tx_array&>(
        *static_cast<const std::any*>(value));
}

} // namespace

using tx_generated::detail::invoke_checked;

extern "C" int txrt_array_concat(const void* left, const void* right,
                                   void** result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::detail::make_handle<std::any>(tx_generated::tx_fn_concat(
            array_value(left), array_value(right)));
    });
}

extern "C" int txrt_array_slice(const void* values, std::int64_t start,
                                  std::int64_t end, void** result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::detail::make_handle<std::any>(tx_generated::tx_fn_array_slice(
            array_value(values), start, end));
    });
}

extern "C" int txrt_array_reverse(const void* values,
                                    void** result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::tx_fn_reverse(array_value(values)));
    });
}

extern "C" int txrt_array_push_back(void* values, const void* value) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_fn_array_push_back(array_value(values),
            *static_cast<const std::any*>(value));
    });
}

extern "C" int txrt_array_pop_back(void* values) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_fn_array_pop_back(array_value(values));
    });
}

extern "C" int txrt_array_insert(void* values, std::int64_t index,
                                 const void* value) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_fn_array_insert(array_value(values), index,
            *static_cast<const std::any*>(value));
    });
}

extern "C" int txrt_array_erase(void* values, std::int64_t index) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_fn_array_erase(array_value(values), index);
    });
}

extern "C" int txrt_array_clear(void* values) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_fn_array_clear(array_value(values));
    });
}
