#include "backend/cpp/call_abi.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <any>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace
{

using tx_generated::tx_array;
using tx_generated::tx_dict;

const std::any& as_value(const void* value)
{
    return *static_cast<const std::any*>(value);
}

std::any& as_value(void* value)
{
    return *static_cast<std::any*>(value);
}

tx_dict& as_keywords(void* value)
{
    auto& item = as_value(value);
    if (item.type() != typeid(tx_dict))
    {
        throw std::runtime_error("命名实参需要字典");
    }
    return std::any_cast<tx_dict&>(item);
}

const tx_dict& as_keywords(const void* value)
{
    const auto& item = as_value(value);
    if (item.type() != typeid(tx_dict))
    {
        throw std::runtime_error("** 展开需要字典");
    }
    return std::any_cast<const tx_dict&>(item);
}

std::string keyword_name(const std::any& key)
{
    if (key.type() != typeid(std::string))
    {
        throw std::runtime_error("** 展开时字典键必须为 str");
    }
    return std::any_cast<const std::string&>(key);
}

bool contains_keyword(const tx_dict& dict, const std::string& name)
{
    return dict.find_value(std::string_view(name)) != nullptr;
}

bool fixed_name(const char* const* names, std::size_t count,
                const std::string& name)
{
    for (std::size_t index = 0; index < count; ++index)
    {
        if (name == names[index])
        {
            return true;
        }
    }
    return false;
}

} // namespace

using tx_generated::detail::invoke_checked;

extern "C" int txrt_keyword_set(void* keywords, const char* name,
                                   const void* value) noexcept
{
    return invoke_checked([&] {
        auto& dict = as_keywords(keywords);
        if (contains_keyword(dict, name))
        {
            throw std::runtime_error("重复命名实参：" + std::string(name));
        }
        dict.emplace_back(std::string(name), as_value(value));
    });
}

extern "C" int txrt_keyword_merge(void* keywords,
                                     const void* source) noexcept
{
    return invoke_checked([&] {
        auto& target = as_keywords(keywords);
        const auto& items = as_keywords(source);
        items.for_each([&](const std::any& key, const std::any& value)
        {
            const auto name = keyword_name(key);
            if (contains_keyword(target, name))
            {
                throw std::runtime_error("重复命名实参：" + name);
            }
            target.emplace_back(name, value);
        });
    });
}

extern "C" int txrt_call_bind(const void* positional,
                                 const void* keywords,
                                 const char* const* names,
                                 std::size_t fixed_count,
                                 int accepts_args, int accepts_kwargs,
                                 void** result) noexcept
{
    return invoke_checked([&] {
        const auto& values = std::any_cast<const tx_array&>(as_value(positional));
        const auto& words = as_keywords(keywords);
        words.for_each([&](const std::any& key, const std::any& value)
        {
            (void)value;
            (void)keyword_name(key);
        });
        if (values.size() > fixed_count && !accepts_args)
        {
            throw std::runtime_error("位置实参数量过多");
        }
        tx_array bound(fixed_count + static_cast<std::size_t>(accepts_args) +
                       static_cast<std::size_t>(accepts_kwargs));
        for (std::size_t index = 0; index < fixed_count; ++index)
        {
            const std::any* named_value = words.find_value(
                std::string_view(names[index]));
            if (index < values.size() && named_value)
            {
                throw std::runtime_error("参数同时收到位置和命名值：" +
                                         std::string(names[index]));
            }
            if (index >= values.size() && !named_value)
            {
                throw std::runtime_error("缺少必需参数：" +
                                         std::string(names[index]));
            }
            bound[index] = named_value ? *named_value : values[index];
        }
        if (accepts_args)
        {
            const auto begin = values.begin() +
                static_cast<std::ptrdiff_t>(fixed_count < values.size()
                    ? fixed_count : values.size());
            bound[fixed_count] = tx_array(begin, values.end());
        }
        tx_dict extra;
        words.for_each([&](const std::any& key, const std::any& value)
        {
            const auto name = keyword_name(key);
            if (!fixed_name(names, fixed_count, name))
            {
                if (!accepts_kwargs)
                {
                    throw std::runtime_error("未知命名实参：" + name);
                }
                extra.emplace_back(name, value);
            }
        });
        if (accepts_kwargs)
        {
            bound[fixed_count + static_cast<std::size_t>(accepts_args)] =
                std::move(extra);
        }
        *result = tx_generated::detail::make_handle<std::any>(std::move(bound));
    });
}

extern "C" int txrt_call_split_spreads(
    const void* spread_array, const void* spread_dict,
    const char* const* fixed_names, std::size_t fixed_count,
    void** args_result, void** kwargs_result) noexcept
{
    return invoke_checked([&]
    {
        const auto& array_value = as_value(spread_array);
        if (array_value.type() != typeid(tx_array))
        {
            throw std::runtime_error("* 展开需要数组");
        }
        const auto& source = std::any_cast<const tx_array&>(array_value);
        const auto& keywords = as_keywords(spread_dict);
        tx_array args(source.begin(), source.end());
        tx_dict kwargs;
        keywords.for_each([&](const std::any& key, const std::any& value)
        {
            const auto name = keyword_name(key);
            if (fixed_name(fixed_names, fixed_count, name))
            {
                throw std::runtime_error("参数同时收到位置和命名值：" + name);
            }
            kwargs.emplace_back(key, value);
        });
        auto* args_handle = tx_generated::detail::make_handle<std::any>(
            std::move(args));
        try
        {
            *kwargs_result = tx_generated::detail::make_handle<std::any>(
                std::move(kwargs));
        }
        catch (...)
        {
            tx_generated::detail::destroy_handle(args_handle);
            throw;
        }
        *args_result = args_handle;
    });
}
