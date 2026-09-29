#include "backend/cpp/value_abi.hpp"
#include "backend/cpp/concurrency_value.hpp"
#include "stdlib/task.hpp"
#include "backend/cpp/vector_value.hpp"
#include "backend/cpp/container_value.hpp"
#include "stdlib/iterator.hpp"
#include "stdlib/closure.hpp"
#include "stdlib/cancellation.hpp"

#include "backend/cpp/runtime.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/file_stream.hpp"
#include "stdlib/encoding_incremental.hpp"
#include "stdlib/regex.hpp"
#include "stdlib/filesystem_watch.hpp"
#include "stdlib/process.hpp"
#include "stdlib/ipc.hpp"
#include "stdlib/json_stream.hpp"
#include "stdlib/cbor.hpp"
#include "stdlib/csv.hpp"
#include "stdlib/xml.hpp"

#include <any>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>

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

template<class field_type>
void* struct_scalar_ptr(const void* value, std::size_t index,
                        const char* type_name) noexcept
{
    const auto* item = static_cast<const std::any*>(value);
    const auto* definition = item ? std::any_cast<dynamic_struct>(item) : nullptr;
    if (definition && index < (*definition)->fields.size())
    {
        auto& field = (*definition)->fields[index].value;
        if (auto* result = std::any_cast<field_type>(&field))
        {
            return result;
        }
    }
    std::snprintf(tx_generated::detail::current_runtime_context().last_error, 256,
                  "结构体字段不是 %s 或索引无效", type_name);
    txrt_require_success(1);
    return nullptr;
}

template<class field_type>
void set_struct_field(void* value, std::size_t index,
                      const char* field_name, field_type&& field)
{
    auto& definition = as_struct(value);
    if (index >= definition->fields.size())
    {
        throw std::runtime_error("结构体字段索引越界");
    }
    definition->fields[index] = {
        field_name, std::forward<field_type>(field)};
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
        *result = make_handle<std::any>(tx_generated::detail::text_value(value));
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
    if (!value)
    {
        return;
    }
    tx_generated::detail::destroy_handle(static_cast<std::any*>(value));
}

extern "C" int txrt_value_assign(void* target, const void* value) noexcept
{
    return invoke_checked([&] { as_value(target) = as_value(value); });
}

extern "C" int txrt_value_set_i64(void* target, std::int64_t value) noexcept
{
    return invoke_checked([&] { as_value(target) = value; });
}

extern "C" int txrt_value_set_f64(void* target, double value) noexcept
{
    return invoke_checked([&] { as_value(target) = value; });
}

extern "C" int txrt_value_set_bool(void* target, bool value) noexcept
{
    return invoke_checked([&] { as_value(target) = value; });
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

extern "C" std::int64_t txrt_value_to_i64_fast(const void* value) noexcept
{
    if (const auto* direct = std::any_cast<std::int64_t>(&as_value(value)))
    {
        return *direct;
    }
    std::int64_t result = 0;
    txrt_require_success(txrt_value_to_i64(value, &result));
    return result;
}

extern "C" double txrt_value_to_f64_fast(const void* value) noexcept
{
    if (const auto* direct = std::any_cast<double>(&as_value(value)))
    {
        return *direct;
    }
    double result = 0;
    txrt_require_success(txrt_value_to_f64(value, &result));
    return result;
}

extern "C" bool txrt_value_to_bool_fast(const void* value) noexcept
{
    if (const auto* direct = std::any_cast<bool>(&as_value(value)))
    {
        return *direct;
    }
    bool result = false;
    txrt_require_success(txrt_value_to_bool(value, &result));
    return result;
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
            (type == "bytes" && item.type() ==
                typeid(tx_generated::byte_value)) ||
            (type == "binary_stream" && item.type() ==
                typeid(tx_generated::binary_stream)) ||
            (type == "text_stream" && item.type() ==
                typeid(tx_generated::text_stream)) ||
            (type == "cancel_source" && item.type() ==
                typeid(tx_generated::cancel_source)) ||
            (type == "cancel_token" && item.type() ==
                typeid(tx_generated::cancel_token)) ||
            tx_generated::concurrency_matches(item, type) ||
            (type == "task_scope" && item.type() ==
                typeid(tx_generated::task_scope_handle)) ||
            (item.type() == typeid(tx_generated::task_handle) &&
             type == std::any_cast<const tx_generated::task_handle&>(item)
                 .type_name) ||
            (type == "encoding_decoder" && item.type() ==
                typeid(tx_generated::encoding_decoder)) ||
            (type == "encoding_encoder" && item.type() ==
                typeid(tx_generated::encoding_encoder)) ||
            (type == "regex_pattern" && item.type() ==
                typeid(tx_generated::regex_pattern)) ||
            (type == "fs_watcher" && item.type() ==
                typeid(tx_generated::fs_watcher)) ||
            (type == "process_child" && item.type() ==
                typeid(tx_generated::process_child)) ||
            (type == "process_pipe" && item.type() ==
                typeid(tx_generated::process_pipe)) ||
            (type == "ipc_listener" && item.type() ==
                typeid(tx_generated::ipc_listener)) ||
            (type == "ipc_stream" && item.type() ==
                typeid(tx_generated::ipc_stream)) ||
            (type == "json_reader" && item.type() == typeid(tx_generated::json_reader)) ||
            (type == "json_writer" && item.type() == typeid(tx_generated::json_writer)) ||
            (type == "cbor_reader" && item.type() == typeid(tx_generated::cbor_reader)) ||
            (type == "cbor_writer" && item.type() == typeid(tx_generated::cbor_writer)) ||
            (type == "csv_reader" && item.type() == typeid(tx_generated::csv_reader)) ||
            (type == "csv_writer" && item.type() == typeid(tx_generated::csv_writer)) ||
            (type == "xml_reader" && item.type() == typeid(tx_generated::xml_reader)) ||
            (type == "xml_writer" && item.type() == typeid(tx_generated::xml_writer)) ||
            (type == "xml_document" && item.type() == typeid(tx_generated::xml_document)) ||
            (type == "xml_node" && item.type() == typeid(tx_generated::xml_node)) ||
            tx_generated::vector_matches(item, type) ||
            (item.type() == typeid(tx_generated::tx_iterator) &&
             type == "iterator<" +
                 std::any_cast<const tx_generated::tx_iterator&>(item)
                     .data().element_type + ">") ||
            (item.type() == typeid(tx_generated::closure_handle) &&
             type == std::any_cast<const tx_generated::closure_handle&>(item)
                         .data().type_name) ||
            tx_generated::container_matches(item, type) ||
            (type == "array" && item.type() == typeid(tx_generated::tx_array)) ||
            (type == "dict" && item.type() == typeid(tx_generated::tx_dict)) ||
            (type == "none" && !item.has_value()) ||
            (item.type() == typeid(dynamic_struct) &&
             std::any_cast<const dynamic_struct&>(item)->type_name == type) ||
            (item.type() == typeid(class_handle) &&
             [&]
             {
                 const auto& object =
                     *std::any_cast<const class_handle&>(item);
                 for (std::size_t index = 0;
                      index < object.ancestor_count; ++index)
                 {
                     if (object.ancestors[index] == type)
                     {
                         return true;
                     }
                 }
                 return false;
             }());
        if (!valid)
            throw std::runtime_error("展开值与目标参数或变量类型不匹配：" + type);
    });
}

extern "C" int txrt_value_require_type_or_none(const void* value,
                                                const char* type_name) noexcept
{
    if (!as_value(value).has_value())
    {
        return 0;
    }
    return txrt_value_require_type(value, type_name);
}

extern "C" int txrt_struct_new(const char* type_name,
                                 const char* display_name,
                                 std::size_t field_count, void** result) noexcept
{
    return invoke_checked([&] {
        *result = make_handle<std::any>(dynamic_struct(dynamic_struct_data{
            type_name, display_name, tx_generated::struct_fields(field_count)}));
    });
}

extern "C" int txrt_struct_set_field(void* value, std::size_t index,
                                       const char* field_name,
                                       const void* field) noexcept
{
    return invoke_checked([&]
    {
        set_struct_field(value, index, field_name, as_value(field));
    });
}

extern "C" int txrt_struct_set_field_i64(void* value, std::size_t index,
                                           const char* field_name,
                                           std::int64_t field) noexcept
{
    return invoke_checked([&]
    {
        set_struct_field(value, index, field_name, field);
    });
}

extern "C" int txrt_struct_set_field_f64(void* value, std::size_t index,
                                           const char* field_name,
                                           double field) noexcept
{
    return invoke_checked([&]
    {
        set_struct_field(value, index, field_name, field);
    });
}

extern "C" int txrt_struct_set_field_bool(void* value, std::size_t index,
                                            const char* field_name,
                                            bool field) noexcept
{
    return invoke_checked([&]
    {
        set_struct_field(value, index, field_name, field);
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
            if (field.name && std::string_view(field.name) == field_name)
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

extern "C" void* txrt_struct_field_i64_ptr(const void* value,
                                             std::size_t index) noexcept
{
    return struct_scalar_ptr<std::int64_t>(value, index, "int");
}

extern "C" void* txrt_struct_field_f64_ptr(const void* value,
                                             std::size_t index) noexcept
{
    return struct_scalar_ptr<double>(value, index, "float");
}

extern "C" void* txrt_struct_field_bool_ptr(const void* value,
                                              std::size_t index) noexcept
{
    return struct_scalar_ptr<bool>(value, index, "bool");
}
