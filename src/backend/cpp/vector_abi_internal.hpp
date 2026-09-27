#pragma once

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/runtime_abi.hpp"
#include "stdlib/array.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace tx_generated::detail
{

template<class element_type>
tx_vector<element_type>& vector_value(const void* value)
{
    auto* result = std::any_cast<tx_vector<element_type>>(
        const_cast<std::any*>(static_cast<const std::any*>(value)));
    if (!result)
    {
        throw std::runtime_error("vector 的实际元素类型不匹配");
    }
    return *result;
}

inline std::size_t vector_count(std::int64_t count)
{
    if (count < 0)
    {
        throw std::out_of_range("vector 长度、容量或下标不能为负");
    }
    return static_cast<std::size_t>(count);
}

template<class element_type>
void* vector_ref(const void* value) noexcept
{
    void* result = nullptr;
    txrt_require_success(invoke_leaf([&]
    {
        result = &vector_value<element_type>(value).data().view;
    }));
    return result;
}

template<class element_type>
int vector_new(std::int64_t count, element_type value, void** result) noexcept
{
    return invoke_checked([&]
    {
        tx_vector<element_type> vector;
        vector.data().values.assign(vector_count(count), value);
        vector.data().refresh();
        *result = make_handle<std::any>(std::move(vector));
    });
}

template<class element_type, class operation>
int vector_modify(void* value, operation&& apply) noexcept
{
    return invoke_checked([&]
    {
        auto& data = vector_value<element_type>(value).data();
        const auto old_capacity = data.values.capacity();
        apply(data.values);
        data.refresh();
        if (data.values.capacity() != old_capacity)
        {
            note_gc_allocation();
        }
    });
}

template<class element_type>
element_type vector_element_from_any(const std::any& value)
{
    if constexpr (std::is_same_v<element_type, text_reference>)
    {
        const auto& text = std::any_cast<const std::string&>(value);
        auto* handle = make_handle<std::string>(text);
        text_reference result(handle);
        destroy_handle(handle);
        return result;
    }
    else if constexpr (std::is_same_v<element_type, std::uint8_t>)
    {
        return std::any_cast<bool>(value);
    }
    else
    {
        return std::any_cast<element_type>(value);
    }
}

template<class element_type>
int vector_from_array(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& source = std::any_cast<const tx_array&>(
            *static_cast<const std::any*>(value));
        tx_vector<element_type> vector;
        auto& values = vector.data().values;
        values.reserve(source.size());
        for (const auto& item : source)
        {
            try
            {
                values.push_back(vector_element_from_any<element_type>(item));
            }
            catch (const std::bad_any_cast&)
            {
                throw std::runtime_error("array 元素与 vector 的元素类型不匹配");
            }
        }
        vector.data().refresh();
        *result = make_handle<std::any>(std::move(vector));
    });
}

template<class element_type>
int vector_to_array(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        tx_array array;
        const auto& values = vector_value<element_type>(value).data().values;
        array.reserve(values.size());
        for (const auto& item : values)
        {
            if constexpr (std::is_same_v<element_type, text_reference>)
            {
                array.push_back(item.get());
            }
            else if constexpr (std::is_same_v<element_type, std::uint8_t>)
            {
                array.push_back(static_cast<bool>(item));
            }
            else
            {
                array.push_back(item);
            }
        }
        *result = make_handle<std::any>(std::move(array));
    });
}

} // namespace tx_generated::detail
