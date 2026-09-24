#include "backend/cpp/value_abi.hpp"

#include "backend/cpp/runtime.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"

#include <any>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

namespace
{

struct dynamic_field
{
    std::string name;
    std::any value;
};

struct dynamic_struct
{
    std::string type_name;
    std::vector<dynamic_field> fields;
};

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

extern "C" int txrt_value_none(void** result) noexcept
{
    return invoke_checked([&] { *result = new std::any; });
}

extern "C" int txrt_value_box_i64(std::int64_t value, void** result) noexcept
{
    return invoke_checked([&] { *result = new std::any(value); });
}

extern "C" int txrt_value_box_f64(double value, void** result) noexcept
{
    return invoke_checked([&] { *result = new std::any(value); });
}

extern "C" int txrt_value_box_bool(bool value, void** result) noexcept
{
    return invoke_checked([&] { *result = new std::any(value); });
}

extern "C" int txrt_value_box_str(const void* value, void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::any(*static_cast<const std::string*>(value));
    });
}

extern "C" int txrt_value_clone(const void* value, void** result) noexcept
{
    return invoke_checked([&] { *result = new std::any(as_value(value)); });
}

extern "C" void txrt_value_release(void* value) noexcept
{
    delete static_cast<std::any*>(value);
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
        *result = new std::string(tx_generated::tx_to_string(as_value(value)));
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

extern "C" int txrt_value_print(const void* value) noexcept
{
    return invoke_checked([&] { tx_generated::tx_print(as_value(value)); });
}

extern "C" int txrt_array_new(std::int64_t length, void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::any(tx_generated::tx_make_array(length));
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
        *result = new std::any(tx_generated::tx_make_array(
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

extern "C" int txrt_struct_new(const char* type_name, std::size_t field_count,
                                 void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::any(dynamic_struct{
            type_name, std::vector<dynamic_field>(field_count)});
    });
}

extern "C" int txrt_struct_set_field(void* value, std::size_t index,
                                       const char* field_name,
                                       const void* field) noexcept
{
    return invoke_checked([&] {
        auto& definition = as_struct(value);
        if (index >= definition.fields.size())
        {
            throw std::runtime_error("结构体字段索引越界");
        }
        definition.fields[index] = {field_name, as_value(field)};
    });
}

extern "C" int txrt_struct_field_address(void* value,
                                           const char* field_name,
                                           void** result) noexcept
{
    return invoke_checked([&] {
        auto& definition = as_struct(value);
        for (auto& field : definition.fields)
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
