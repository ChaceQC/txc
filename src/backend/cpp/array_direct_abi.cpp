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
        *result = new std::any(tx_generated::tx_fn_concat(
            array_value(left), array_value(right)));
    });
}

extern "C" int txrt_array_slice(const void* values, std::int64_t start,
                                  std::int64_t end, void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::any(tx_generated::tx_fn_array_slice(
            array_value(values), start, end));
    });
}

extern "C" int txrt_array_reverse(const void* values,
                                    void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::any(tx_generated::tx_fn_reverse(array_value(values)));
    });
}
