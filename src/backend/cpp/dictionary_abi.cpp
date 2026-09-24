#include "backend/cpp/value_abi.hpp"

#include "backend/cpp/runtime.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"

#include <any>
#include <limits>
#include <stdexcept>
#include <string>
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

bool equal_key(const std::any& left, const std::any& right)
{
    if (left.type() != right.type())
    {
        return false;
    }
    if (!left.has_value())
    {
        return true;
    }
    if (left.type() == typeid(std::int64_t))
    {
        return std::any_cast<std::int64_t>(left) ==
               std::any_cast<std::int64_t>(right);
    }
    if (left.type() == typeid(double))
    {
        return std::any_cast<double>(left) == std::any_cast<double>(right);
    }
    if (left.type() == typeid(bool))
    {
        return std::any_cast<bool>(left) == std::any_cast<bool>(right);
    }
    return std::any_cast<const std::string&>(left) ==
           std::any_cast<const std::string&>(right);
}

std::any* find_value(tx_dict& dict, const std::any& key)
{
    for (auto& [entry_key, value] : dict)
    {
        if (equal_key(entry_key, key))
        {
            return &value;
        }
    }
    return nullptr;
}

} // namespace

using tx_generated::detail::invoke_checked;

extern "C" int txrt_dict_new(void** result) noexcept
{
    return invoke_checked([&] { *result = new std::any(tx_dict{}); });
}

extern "C" int txrt_dict_set(void* value, const void* key,
                               const void* item) noexcept
{
    return invoke_checked([&] {
        auto& dict = as_dict(value);
        const auto& actual_key = as_value(key);
        require_hashable(actual_key);
        if (auto* found = find_value(dict, actual_key))
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

extern "C" int txrt_dict_key_address(void* value, std::int64_t index,
                                        void** result) noexcept
{
    return invoke_checked([&] {
        auto& dict = as_dict(value);
        if (index < 0 || static_cast<std::size_t>(index) >= dict.size())
        {
            throw std::runtime_error("字典索引越界");
        }
        *result = &dict[static_cast<std::size_t>(index)].first;
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
        require_hashable(actual_key);
        if (auto* found = find_value(dict, actual_key))
        {
            *result = found;
            return;
        }
        if (!create)
        {
            throw std::runtime_error("字典键不存在");
        }
        dict.emplace_back(actual_key, std::any{});
        *result = &dict.back().second;
    });
}
