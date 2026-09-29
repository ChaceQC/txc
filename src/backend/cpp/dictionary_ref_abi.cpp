#include "backend/cpp/value_abi.hpp"

#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <any>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{

using tx_generated::tx_dict;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

template<class key_type>
const std::any& find_required(const void* value, const key_type& key)
{
    const auto& dictionary = *static_cast<const tx_dict*>(value);
    const auto* found = dictionary.find_value(key);
    if (!found)
    {
        throw std::runtime_error("字典键不存在");
    }
    return *found;
}

template<class key_type>
const void* read_ptr(const void* value, const key_type& key) noexcept
{
    const void* result = nullptr;
    txrt_require_success(invoke_checked([&]
    {
        result = &find_required(value, key);
    }));
    return result;
}

template<class key_type>
std::int64_t read_i64(const void* value, const key_type& key) noexcept
{
    std::int64_t result = 0;
    txrt_require_success(invoke_checked([&]
    {
        result = tx_generated::tx_to_int(find_required(value, key));
    }));
    return result;
}

template<class key_type>
double read_f64(const void* value, const key_type& key) noexcept
{
    double result = 0;
    txrt_require_success(invoke_checked([&]
    {
        result = tx_generated::tx_to_float(find_required(value, key));
    }));
    return result;
}

template<class key_type>
void* read_str(const void* value, const key_type& key) noexcept
{
    void* result = nullptr;
    txrt_require_success(invoke_checked([&]
    {
        result = make_handle<std::string>(
            tx_generated::tx_to_string(find_required(value, key)));
    }));
    return result;
}

} // namespace

extern "C" void* txrt_dict_ref(void* value) noexcept
{
    if (auto* dictionary = std::any_cast<tx_dict>(
            static_cast<std::any*>(value)))
    {
        return dictionary;
    }
    std::snprintf(tx_generated::detail::current_runtime_context().last_error, 256, "对象不是字典");
    txrt_require_success(1);
    return nullptr;
}

extern "C" std::int64_t txrt_dict_ref_len(const void* value) noexcept
{
    std::int64_t result = 0;
    txrt_require_success(invoke_checked([&]
    {
        const auto size = static_cast<const tx_dict*>(value)->size();
        if (size > static_cast<std::size_t>(
                std::numeric_limits<std::int64_t>::max()))
        {
            throw std::runtime_error("字典长度超出 int 范围");
        }
        result = static_cast<std::int64_t>(size);
    }));
    return result;
}

extern "C" const void* txrt_dict_ref_element_read_ptr(
    const void* value, const void* key) noexcept
{
    return read_ptr(value, *static_cast<const std::any*>(key));
}

extern "C" const void* txrt_dict_ref_element_read_ptr_str(
    const void* value, const void* key) noexcept
{
    return read_ptr(value, std::string_view(
        tx_generated::detail::text_value(key)));
}

extern "C" const void* txrt_dict_ref_element_read_ptr_literal(
    const void* value, const char* key, std::size_t length) noexcept
{
    return read_ptr(value, std::string_view(key, length));
}

extern "C" std::int64_t txrt_dict_ref_get_i64(
    const void* value, const void* key) noexcept
{
    return read_i64(value, *static_cast<const std::any*>(key));
}

extern "C" std::int64_t txrt_dict_ref_get_i64_str(
    const void* value, const void* key) noexcept
{
    return read_i64(value, std::string_view(
        tx_generated::detail::text_value(key)));
}

extern "C" std::int64_t txrt_dict_ref_get_i64_literal(
    const void* value, const char* key, std::size_t length) noexcept
{
    return read_i64(value, std::string_view(key, length));
}

extern "C" double txrt_dict_ref_get_f64(
    const void* value, const void* key) noexcept
{
    return read_f64(value, *static_cast<const std::any*>(key));
}

extern "C" double txrt_dict_ref_get_f64_str(
    const void* value, const void* key) noexcept
{
    return read_f64(value, std::string_view(
        tx_generated::detail::text_value(key)));
}

extern "C" double txrt_dict_ref_get_f64_literal(
    const void* value, const char* key, std::size_t length) noexcept
{
    return read_f64(value, std::string_view(key, length));
}

extern "C" void* txrt_dict_ref_get_str(
    const void* value, const void* key) noexcept
{
    return read_str(value, *static_cast<const std::any*>(key));
}

extern "C" void* txrt_dict_ref_get_str_str(
    const void* value, const void* key) noexcept
{
    return read_str(value, std::string_view(
        tx_generated::detail::text_value(key)));
}

extern "C" void* txrt_dict_ref_get_str_literal(
    const void* value, const char* key, std::size_t length) noexcept
{
    return read_str(value, std::string_view(key, length));
}
