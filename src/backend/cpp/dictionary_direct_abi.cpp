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
using tx_generated::detail::invoke_leaf;
using tx_generated::detail::make_handle;

extern "C" int txrt_dictionary_get(const void* values, const void* key,
                                     void** result) noexcept
{
    return invoke_leaf([&]
    {
        *result = make_handle<std::any>(tx_generated::tx_fn_dictionary_get(
            dictionary_value(values), key_value(key)));
    });
}

extern "C" int txrt_dictionary_get_str(const void* values, const void* key,
                                         void** result) noexcept
{
    return invoke_leaf([&]
    {
        const auto* found = dictionary_value(values).find_value(
            std::string_view(*static_cast<const std::string*>(key)));
        *result = make_handle<std::any>(found ? *found : std::any{});
    });
}

extern "C" int txrt_dictionary_contains(const void* values, const void* key,
                                          bool* result) noexcept
{
    return invoke_leaf([&]
    {
        *result = tx_generated::tx_fn_dictionary_contains(
            dictionary_value(values), key_value(key));
    });
}

extern "C" int txrt_dictionary_contains_str(const void* values,
    const void* key, bool* result) noexcept
{
    return invoke_leaf([&]
    {
        *result = dictionary_value(values).find_value(
            std::string_view(*static_cast<const std::string*>(key))) != nullptr;
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

extern "C" int txrt_dictionary_remove_str(void* values,
    const void* key, bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = dictionary_value(values).erase(
            std::string_view(*static_cast<const std::string*>(key)));
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

extern "C" int txrt_dictionary_items(const void* values, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::tx_fn_dictionary_items(
            dictionary_value(values)));
    });
}
