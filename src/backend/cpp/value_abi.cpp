#include "backend/cpp/value_abi.hpp"

#include "backend/cpp/runtime.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"

#include <any>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

namespace
{

using tx_generated::dynamic_field;
using tx_generated::dynamic_struct;
using tx_generated::dynamic_struct_data;
using tx_generated::class_handle;

std::any& as_value(void* value)
{
    return *static_cast<std::any*>(value);
}

const std::any& as_value(const void* value)
{
    return *static_cast<const std::any*>(value);
}

dynamic_struct& as_struct(void* value)
{
    auto& item = as_value(value);
    if (item.type() != typeid(dynamic_struct))
    {
        throw std::runtime_error("字段访问需要结构体类型");
    }
    return std::any_cast<dynamic_struct&>(item);
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_value_none(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>();
    });
}

extern "C" int txrt_value_box_i64(std::int64_t value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(value);
    });
}

extern "C" int txrt_value_box_f64(double value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(value);
    });
}

extern "C" int txrt_value_box_bool(bool value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(value);
    });
}

extern "C" int txrt_value_box_str(const void* value, void** result) noexcept
{
    return invoke_checked([&] {
        *result = make_handle<std::any>(*static_cast<const std::string*>(value));
    });
}

extern "C" int txrt_value_clone(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(as_value(value));
    });
}

extern "C" void txrt_value_release(void* value) noexcept
{
    tx_generated::detail::destroy_handle(static_cast<std::any*>(value));
}

extern "C" int txrt_value_assign(void* target, const void* value) noexcept
{
    return invoke_checked([&] { as_value(target) = as_value(value); });
}

extern "C" int txrt_value_to_i64(const void* value,
                                   std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_to_int(as_value(value)); });
}

extern "C" int txrt_value_to_f64(const void* value, double* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_to_float(as_value(value)); });
}

extern "C" int txrt_value_to_bool(const void* value, bool* result) noexcept
{
    return invoke_checked([&] {
        const auto& item = as_value(value);
        if (item.type() != typeid(bool))
        {
            throw std::runtime_error("数组元素不是 bool");
        }
        *result = std::any_cast<const bool&>(item);
    });
}

extern "C" int txrt_value_to_str(const void* value, void** result) noexcept
{
    return invoke_checked([&] {
        *result = make_handle<std::string>(tx_generated::tx_to_string(as_value(value)));
    });
}

extern "C" int txrt_value_is_none(const void* value, bool* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_is_none(as_value(value)); });
}

extern "C" int txrt_value_len(const void* value,
                               std::int64_t* result) noexcept
{
    return invoke_checked([&] { *result = tx_generated::tx_len(as_value(value)); });
}

extern "C" int txrt_value_print(const void* value, bool newline) noexcept
{
    return invoke_checked([&] {
        auto text = tx_generated::format_print_value(as_value(value));
        if (newline)
        {
            tx_generated::tx_fn_write_line(std::move(text));
        }
        else
        {
            tx_generated::tx_fn_write(std::move(text));
        }
    });
}

extern "C" int txrt_value_require_type(const void* value,
                                         const char* type_name) noexcept
{
    return invoke_checked([&] {
        const auto& item = as_value(value);
        const std::string type(type_name);
        const bool valid = type == "any" ||
            (type == "int" && item.type() == typeid(std::int64_t)) ||
            (type == "float" && item.type() == typeid(double)) ||
            (type == "bool" && item.type() == typeid(bool)) ||
            (type == "str" && item.type() == typeid(std::string)) ||
            (type == "array" && item.type() == typeid(tx_generated::tx_array)) ||
            (type == "dict" && item.type() == typeid(tx_generated::tx_dict)) ||
            (type == "none" && !item.has_value()) ||
            (item.type() == typeid(dynamic_struct) &&
             std::any_cast<const dynamic_struct&>(item)->type_name == type) ||
            (item.type() == typeid(class_handle) &&
             [&]
             {
                 const auto& ancestors =
                     std::any_cast<const class_handle&>(item)->ancestors;
                 return std::find(ancestors.begin(), ancestors.end(), type) !=
                        ancestors.end();
             }());
        if (!valid)
            throw std::runtime_error("展开值与目标参数或变量类型不匹配：" + type);
    });
}

extern "C" int txrt_array_new(std::int64_t length, void** result) noexcept
{
    return invoke_checked([&] {
        *result = make_handle<std::any>(tx_generated::tx_make_array(length));
    });
}

extern "C" int txrt_array_resize(std::int64_t length, const void* initial,
                                   void** result) noexcept
{
    return invoke_checked([&] {
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
    return invoke_checked([&] {
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
    return invoke_checked([&] {
        auto& item = as_value(value);
        if (item.type() != typeid(tx_generated::tx_array))
        {
            throw std::runtime_error("索引对象不是数组");
        }
        *result = &tx_generated::tx_at(
            std::any_cast<tx_generated::tx_array&>(item), index);
    });
}

extern "C" int txrt_array_append(void* value, const void* item) noexcept
{
    return invoke_checked([&] {
        auto& values = std::any_cast<tx_generated::tx_array&>(as_value(value));
        values.push_back(as_value(item));
    });
}

extern "C" int txrt_array_extend(void* value, const void* items) noexcept
{
    return invoke_checked([&] {
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

extern "C" int txrt_array_require_length(const void* value,
                                            std::size_t length) noexcept
{
    return invoke_checked([&] {
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

extern "C" int txrt_struct_new(const char* type_name,
                                 const char* display_name,
                                 std::size_t field_count, void** result) noexcept
{
    return invoke_checked([&] {
        *result = make_handle<std::any>(dynamic_struct(dynamic_struct_data{
            type_name, display_name, std::vector<dynamic_field>(field_count)}));
    });
}

extern "C" int txrt_struct_set_field(void* value, std::size_t index,
                                       const char* field_name,
                                       const void* field) noexcept
{
    return invoke_checked([&] {
        auto& definition = as_struct(value);
        if (index >= definition->fields.size())
        {
            throw std::runtime_error("结构体字段索引越界");
        }
        definition->fields[index] = {field_name, as_value(field)};
    });
}

extern "C" int txrt_struct_field_address(void* value,
                                           const char* field_name,
                                           void** result) noexcept
{
    return invoke_checked([&] {
        auto& definition = as_struct(value);
        for (auto& field : definition->fields)
        {
            if (field.name == field_name)
            {
                *result = &field.value;
                return;
            }
        }
        throw std::runtime_error("字段不存在或对象不是结构体");
    });
}

extern "C" int txrt_struct_field_address_index(void* value,
                                                 std::size_t index,
                                                 void** result) noexcept
{
    return invoke_checked([&] {
        auto& definition = as_struct(value);
        if (index >= definition->fields.size())
        {
            throw std::runtime_error("结构体字段索引越界");
        }
        *result = &definition->fields[index].value;
    });
}
