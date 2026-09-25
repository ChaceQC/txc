#include "backend/cpp/value_abi.hpp"

#include "backend/cpp/runtime.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"

#include <any>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>

namespace
{

using tx_generated::tx_array;
using tx_generated::tx_dict;

std::any& as_value(void* value)
{
    return *static_cast<std::any*>(value);
}

const std::any& as_value(const void* value)
{
    return *static_cast<const std::any*>(value);
}

tx_dict& as_dict(void* value)
{
    auto& item = as_value(value);
    if (item.type() != typeid(tx_dict))
    {
        throw std::runtime_error("索引对象不是字典");
    }
    return std::any_cast<tx_dict&>(item);
}

const tx_dict& as_dict(const void* value)
{
    const auto& item = as_value(value);
    if (item.type() != typeid(tx_dict))
    {
        throw std::runtime_error("对象不是字典");
    }
    return std::any_cast<const tx_dict&>(item);
}

void require_hashable(const std::any& key)
{
    if (!key.has_value() || key.type() == typeid(std::int64_t) ||
        key.type() == typeid(double) || key.type() == typeid(bool) ||
        key.type() == typeid(std::string))
    {
        return;
    }
    throw std::runtime_error("字典键必须是可哈希的值");
}

std::any* dict_element_address(tx_dict& dict, const std::any& key, bool create)
{
    require_hashable(key);
    if (auto* found = dict.find_value(key))
    {
        return found;
    }
    if (!create)
    {
        throw std::runtime_error("字典键不存在");
    }
    return &dict.emplace_back(key, std::any{});
}

} // namespace

using tx_generated::detail::invoke_checked;

extern "C" int txrt_dict_new(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(tx_dict{});
    });
}

extern "C" int txrt_dict_set(void* value, const void* key,
                               const void* item) noexcept
{
    return invoke_checked([&] {
        auto& dict = as_dict(value);
        const auto& actual_key = as_value(key);
        require_hashable(actual_key);
        if (auto* found = dict.find_value(actual_key))
        {
            *found = as_value(item);
        }
        else
        {
            dict.emplace_back(actual_key, as_value(item));
        }
    });
}

extern "C" int txrt_dict_len(const void* value,
                                std::int64_t* result) noexcept
{
    return invoke_checked([&] {
        const auto size = as_dict(value).size();
        if (size > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()))
        {
            throw std::runtime_error("字典长度超出 int 范围");
        }
        *result = static_cast<std::int64_t>(size);
    });
}

extern "C" int txrt_value_element_address(void* value, const void* key,
                                             bool create, void** result) noexcept
{
    return invoke_checked([&] {
        auto& object = as_value(value);
        const auto& actual_key = as_value(key);
        if (object.type() == typeid(tx_array))
        {
            if (actual_key.type() != typeid(std::int64_t))
            {
                throw std::runtime_error("数组索引需要 int");
            }
            auto& values = std::any_cast<tx_array&>(object);
            *result = &tx_generated::tx_at(
                values, std::any_cast<std::int64_t>(actual_key));
            return;
        }
        if (object.type() != typeid(tx_dict))
        {
            throw std::runtime_error("索引对象不是数组或字典");
        }
        auto& dict = std::any_cast<tx_dict&>(object);
        *result = dict_element_address(dict, actual_key, create);
    });
}

extern "C" int txrt_dict_element_address(void* value, const void* key,
                                            bool create, void** result) noexcept
{
    return invoke_checked([&] {
        *result = dict_element_address(as_dict(value), as_value(key), create);
    });
}

extern "C" int txrt_dict_element_address_str(void* value, const void* key,
                                                bool create, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = dict_element_address(as_dict(value),
            *static_cast<const std::string*>(key), create);
    });
}

extern "C" int txrt_dict_element_address_literal(
    void* value, const char* key, std::size_t length, bool create,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        auto& dict = as_dict(value);
        const std::string_view text(key, length);
        if (auto* found = dict.find_value(text))
        {
            *result = found;
            return;
        }
        if (!create)
        {
            throw std::runtime_error("字典键不存在");
        }
        *result = &dict.emplace_back(std::string(text), std::any{});
    });
}
