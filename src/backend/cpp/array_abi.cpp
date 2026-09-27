#include "backend/cpp/value_abi.hpp"

#include "backend/cpp/runtime.hpp"
#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <any>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <typeinfo>

namespace
{

std::any& as_value(void* value)
{
    return *static_cast<std::any*>(value);
}

const std::any& as_value(const void* value)
{
    return *static_cast<const std::any*>(value);
}

tx_generated::tx_array& array_for_write(void* value)
{
    auto* array = std::any_cast<tx_generated::tx_array>(&as_value(value));
    if (!array)
    {
        throw std::runtime_error("索引对象不是数组");
    }
    return *array;
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_array_new(std::int64_t length, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::tx_make_array(length));
    });
}

extern "C" void* txrt_array_ref(void* value) noexcept
{
    if (auto* array = std::any_cast<tx_generated::tx_array>(&as_value(value)))
    {
        return array;
    }
    std::snprintf(tx_generated::detail::current_runtime_context().last_error, 256, "对象不是数组");
    txrt_require_success(1);
    return nullptr;
}

extern "C" void txrt_array_index_error() noexcept
{
    std::snprintf(tx_generated::detail::current_runtime_context().last_error, 256, "数组索引越界");
    txrt_require_success(1);
}

extern "C" std::int64_t txrt_array_ref_len(const void* value) noexcept
{
    std::int64_t result = 0;
    txrt_require_success(invoke_checked([&]
    {
        result = tx_generated::tx_len(
            *static_cast<const tx_generated::tx_array*>(value));
    }));
    return result;
}

extern "C" std::int64_t txrt_array_ref_get_i64(const void* value,
                                                 std::int64_t index) noexcept
{
    const auto& array = *static_cast<const tx_generated::tx_array*>(value);
    if (index >= 0 && static_cast<std::uint64_t>(index) < array.size())
    {
        if (const auto* integer = std::any_cast<std::int64_t>(
                &array[static_cast<std::size_t>(index)]))
        {
            return *integer;
        }
    }
    std::int64_t result = 0;
    txrt_require_success(invoke_checked([&]
    {
        result = tx_generated::tx_to_int(tx_generated::tx_at(array, index));
    }));
    return result;
}

extern "C" double txrt_array_ref_get_f64(const void* value,
                                           std::int64_t index) noexcept
{
    const auto& array = *static_cast<const tx_generated::tx_array*>(value);
    if (index >= 0 && static_cast<std::uint64_t>(index) < array.size())
    {
        if (const auto* number = std::any_cast<double>(
                &array[static_cast<std::size_t>(index)]))
        {
            return *number;
        }
    }
    double result = 0;
    txrt_require_success(invoke_checked([&]
    {
        result = tx_generated::tx_to_float(tx_generated::tx_at(array, index));
    }));
    return result;
}

extern "C" void* txrt_array_ref_get_str(const void* value,
                                          std::int64_t index) noexcept
{
    const auto& array = *static_cast<const tx_generated::tx_array*>(value);
    void* result = nullptr;
    txrt_require_success(invoke_checked([&]
    {
        const auto& element = tx_generated::tx_at(array, index);
        if (const auto* text = std::any_cast<std::string>(&element))
        {
            result = make_handle<std::string>(*text);
        }
        else
        {
            result = make_handle<std::string>(
                tx_generated::tx_to_string(element));
        }
    }));
    return result;
}

extern "C" const void* txrt_array_ref_element_read_ptr(
    const void* value, std::int64_t index) noexcept
{
    const void* result = nullptr;
    txrt_require_success(invoke_checked([&]
    {
        result = &tx_generated::tx_at(
            *static_cast<const tx_generated::tx_array*>(value), index);
    }));
    return result;
}

extern "C" void* txrt_array_ref_element_ptr(void* value,
                                              std::int64_t index) noexcept
{
    void* result = nullptr;
    txrt_require_success(invoke_checked([&]
    {
        result = &tx_generated::tx_at(
            *static_cast<tx_generated::tx_array*>(value), index);
    }));
    return result;
}

extern "C" int txrt_array_resize(std::int64_t length, const void* initial,
                                   void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& source = as_value(initial);
        if (source.type() != typeid(tx_generated::tx_array))
        {
            throw std::runtime_error("数组初值需要数组类型");
        }
        *result = make_handle<std::any>(tx_generated::tx_make_array(
            length, std::any_cast<const tx_generated::tx_array&>(source)));
    });
}

extern "C" int txrt_array_len(const void* value,
                               std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        const auto& item = as_value(value);
        if (item.type() != typeid(tx_generated::tx_array))
        {
            throw std::runtime_error("len 的对象不是数组");
        }
        *result = tx_generated::tx_len(
            std::any_cast<const tx_generated::tx_array&>(item));
    });
}

extern "C" int txrt_array_element_address(void* value, std::int64_t index,
                                            void** result) noexcept
{
    return invoke_checked([&]
    {
        auto& item = as_value(value);
        if (item.type() != typeid(tx_generated::tx_array))
        {
            throw std::runtime_error("索引对象不是数组");
        }
        *result = &tx_generated::tx_at(
            std::any_cast<tx_generated::tx_array&>(item), index);
    });
}

extern "C" void* txrt_array_element_ptr(void* value,
                                          std::int64_t index) noexcept
{
    if (auto* array = std::any_cast<tx_generated::tx_array>(&as_value(value));
        array && index >= 0 &&
        static_cast<std::size_t>(index) < array->size())
    {
        return &(*array)[static_cast<std::size_t>(index)];
    }
    void* result = nullptr;
    txrt_require_success(txrt_array_element_address(value, index, &result));
    return result;
}

extern "C" const void* txrt_array_element_read_ptr(const void* value,
                                                     std::int64_t index) noexcept
{
    if (const auto* array = std::any_cast<tx_generated::tx_array>(&as_value(value));
        array && index >= 0 &&
        static_cast<std::size_t>(index) < array->size())
    {
        return &(*array)[static_cast<std::size_t>(index)];
    }
    const void* result = nullptr;
    txrt_require_success(invoke_checked([&]
    {
        result = &tx_generated::tx_at(as_value(value), index);
    }));
    return result;
}

namespace
{

std::size_t scalar_index(const tx_generated::tx_array& array,
                         std::int64_t index)
{
    if (index < 0 || static_cast<std::uint64_t>(index) >= array.size())
    {
        throw std::out_of_range("数组索引越界");
    }
    return static_cast<std::size_t>(index);
}

} // namespace

extern "C" int txrt_array_ref_set_i64(void* value, std::int64_t index,
                                        std::int64_t item) noexcept
{
    return invoke_checked([&]
    {
        auto& array = *static_cast<tx_generated::tx_array*>(value);
        array.set_scalar(scalar_index(array, index), item);
    });
}

extern "C" int txrt_array_ref_set_f64(void* value, std::int64_t index,
                                        double item) noexcept
{
    return invoke_checked([&]
    {
        auto& array = *static_cast<tx_generated::tx_array*>(value);
        array.set_scalar(scalar_index(array, index), item);
    });
}

extern "C" int txrt_array_ref_set_bool(void* value, std::int64_t index,
                                         bool item) noexcept
{
    return invoke_checked([&]
    {
        auto& array = *static_cast<tx_generated::tx_array*>(value);
        array.set_scalar(scalar_index(array, index), item);
    });
}

extern "C" int txrt_array_ref_set_str(void* value, std::int64_t index,
                                        const void* item) noexcept
{
    return invoke_checked([&]
    {
        auto& array = *static_cast<tx_generated::tx_array*>(value);
        array.set_text(scalar_index(array, index),
                       *static_cast<const std::string*>(item));
    });
}

extern "C" int txrt_array_set_i64(void* value, std::int64_t index,
                                    std::int64_t item) noexcept
{
    return invoke_checked([&]
    {
        auto& array = array_for_write(value);
        array.set_scalar(scalar_index(array, index), item);
    });
}

extern "C" int txrt_array_set_f64(void* value, std::int64_t index,
                                    double item) noexcept
{
    return invoke_checked([&]
    {
        auto& array = array_for_write(value);
        array.set_scalar(scalar_index(array, index), item);
    });
}

extern "C" int txrt_array_set_bool(void* value, std::int64_t index,
                                     bool item) noexcept
{
    return invoke_checked([&]
    {
        auto& array = array_for_write(value);
        array.set_scalar(scalar_index(array, index), item);
    });
}

extern "C" int txrt_array_set_str(void* value, std::int64_t index,
                                    const void* item) noexcept
{
    return invoke_checked([&]
    {
        auto& array = array_for_write(value);
        array.set_text(scalar_index(array, index),
                       *static_cast<const std::string*>(item));
    });
}

extern "C" int txrt_array_set_value(void* value, std::int64_t index,
                                      const void* item) noexcept
{
    return invoke_checked([&]
    {
        auto& array = array_for_write(value);
        array.set_value(scalar_index(array, index),
                        *static_cast<const std::any*>(item));
    });
}

extern "C" int txrt_array_append(void* value, const void* item) noexcept
{
    return invoke_checked([&]
    {
        auto& values = std::any_cast<tx_generated::tx_array&>(as_value(value));
        values.push_back(as_value(item));
    });
}

extern "C" int txrt_array_extend(void* value, const void* items) noexcept
{
    return invoke_checked([&]
    {
        auto& values = std::any_cast<tx_generated::tx_array&>(as_value(value));
        const auto& source = as_value(items);
        if (source.type() != typeid(tx_generated::tx_array))
        {
            throw std::runtime_error("* 展开需要数组");
        }
        const auto& more = std::any_cast<const tx_generated::tx_array&>(source);
        if (values.identity() == more.identity())
        {
            const tx_generated::tx_array snapshot(more.begin(), more.end());
            values.insert(values.end(), snapshot.begin(), snapshot.end());
        }
        else
        {
            values.insert(values.end(), more.begin(), more.end());
        }
    });
}

extern "C" void txrt_array_require_spread(const void* value) noexcept
{
    if (value &&
        as_value(value).type() == typeid(tx_generated::tx_array))
    {
        return;
    }
    std::snprintf(tx_generated::detail::current_runtime_context().last_error, 256,
                  "* 展开需要数组");
    txrt_require_success(1);
}

extern "C" int txrt_array_require_length(const void* value,
                                            std::size_t length) noexcept
{
    return invoke_checked([&]
    {
        const auto& source = as_value(value);
        if (source.type() != typeid(tx_generated::tx_array))
        {
            throw std::runtime_error("解包右侧需要数组");
        }
        const auto& values = std::any_cast<const tx_generated::tx_array&>(source);
        if (values.size() != length)
        {
            throw std::runtime_error("解包数量与数组长度不匹配");
        }
    });
}
