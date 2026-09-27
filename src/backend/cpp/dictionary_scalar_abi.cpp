#include "backend/cpp/external_direct_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/dictionary.hpp"

namespace
{

using tx_generated::tx_dict;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::invoke_leaf;
using tx_generated::detail::make_handle;

const tx_dict& dictionary_value(const void* value)
{
    return std::any_cast<const tx_dict&>(*static_cast<const std::any*>(value));
}

tx_dict& dictionary_value(void* value)
{
    return std::any_cast<tx_dict&>(*static_cast<std::any*>(value));
}

template<class key_type>
int scalar_get(const void* values, key_type key, void** result) noexcept
{
    return invoke_leaf([&]
    {
        const auto* found = dictionary_value(values).find_value(key);
        *result = make_handle<std::any>(found ? *found : std::any{});
    });
}

template<class key_type>
int scalar_contains(const void* values, key_type key, bool* result) noexcept
{
    return invoke_leaf([&]
    {
        *result = dictionary_value(values).find_value(key) != nullptr;
    });
}

template<class key_type>
int scalar_remove(void* values, key_type key, bool* result) noexcept
{
    // 字典值可能是用户对象；删除必须传播析构产生的 TX 错误。
    return invoke_checked([&]
    {
        *result = dictionary_value(values).erase(key);
    });
}

} // namespace

extern "C" int txrt_dictionary_get_i64(const void* values,
    std::int64_t key, void** result) noexcept
{
    return scalar_get(values, key, result);
}

extern "C" int txrt_dictionary_contains_i64(const void* values,
    std::int64_t key, bool* result) noexcept
{
    return scalar_contains(values, key, result);
}

extern "C" int txrt_dictionary_remove_i64(void* values,
    std::int64_t key, bool* result) noexcept
{
    return scalar_remove(values, key, result);
}

extern "C" int txrt_dictionary_get_f64(const void* values,
    double key, void** result) noexcept
{
    return scalar_get(values, key, result);
}

extern "C" int txrt_dictionary_contains_f64(const void* values,
    double key, bool* result) noexcept
{
    return scalar_contains(values, key, result);
}

extern "C" int txrt_dictionary_remove_f64(void* values,
    double key, bool* result) noexcept
{
    return scalar_remove(values, key, result);
}

extern "C" int txrt_dictionary_get_bool(const void* values,
    bool key, void** result) noexcept
{
    return scalar_get(values, key, result);
}

extern "C" int txrt_dictionary_contains_bool(const void* values,
    bool key, bool* result) noexcept
{
    return scalar_contains(values, key, result);
}

extern "C" int txrt_dictionary_remove_bool(void* values,
    bool key, bool* result) noexcept
{
    return scalar_remove(values, key, result);
}
