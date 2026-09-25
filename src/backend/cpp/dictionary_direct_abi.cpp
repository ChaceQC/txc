#include "backend/cpp/external_direct_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <any>

namespace
{

const tx_generated::tx_dict& dictionary_value(const void* value)
{
    return std::any_cast<const tx_generated::tx_dict&>(
        *static_cast<const std::any*>(value));
}

tx_generated::tx_dict dictionary_value(void* value)
{
    return std::any_cast<tx_generated::tx_dict>(
        *static_cast<std::any*>(value));
}

const std::any& key_value(const void* value)
{
    return *static_cast<const std::any*>(value);
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_dictionary_get(const void* values, const void* key,
                                     void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::tx_fn_dictionary_get(
            dictionary_value(values), key_value(key)));
    });
}

extern "C" int txrt_dictionary_contains(const void* values, const void* key,
                                          bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::tx_fn_dictionary_contains(
            dictionary_value(values), key_value(key));
    });
}

extern "C" int txrt_dictionary_remove(void* values, const void* key,
                                        bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::tx_fn_dictionary_remove(
            dictionary_value(values), key_value(key));
    });
}

extern "C" int txrt_dictionary_keys(const void* values,
                                      void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::tx_fn_dictionary_keys(
            dictionary_value(values)));
    });
}

extern "C" int txrt_dictionary_values(const void* values,
                                        void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::tx_fn_dictionary_values(
            dictionary_value(values)));
    });
}

extern "C" int txrt_dictionary_clear(void* values) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_fn_dictionary_clear(dictionary_value(values));
    });
}
