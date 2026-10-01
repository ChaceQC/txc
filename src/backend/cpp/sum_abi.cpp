#include "backend/cpp/sum_abi.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/value_format.hpp"

#include <any>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{

using tx_generated::dynamic_struct;
using tx_generated::dynamic_struct_data;
using tx_generated::struct_fields;
using tx_generated::runtime_failure;

const dynamic_struct& require_sum(const void* value, std::string_view prefix)
{
    const auto& item = *static_cast<const std::any*>(value);
    const auto* result = std::any_cast<dynamic_struct>(&item);
    if (!result || !std::string_view((*result)->type_name).starts_with(prefix))
    {
        throw std::runtime_error("option/result 的运行时类型不匹配");
    }
    return *result;
}

bool state(const dynamic_struct& value)
{
    return std::any_cast<bool>(value->fields[0].value);
}

tx::error_kind parse_kind(std::string_view value)
{
    if (value == "parse_error") return tx::error_kind::parse;
    if (value == "io_error") return tx::error_kind::io;
    if (value == "process_error") return tx::error_kind::process;
    if (value == "database_error") return tx::error_kind::database;
    if (value == "security_error") return tx::error_kind::security;
    if (value == "cancelled_error") return tx::error_kind::cancelled;
    if (value == "runtime_error") return tx::error_kind::runtime;
    if (value == "graphics_error")
    {
        return tx::error_kind::graphics;
    }
    throw std::runtime_error("result 错误类别无效");
}

tx_generated::error_info read_error(const std::any& value)
{
    const auto& info = std::any_cast<const dynamic_struct&>(value);
    if (info->fields.size() != 3)
    {
        throw std::runtime_error("result 错误信息字段不完整");
    }
    return {
        parse_kind(std::any_cast<const std::string&>(info->fields[0].value)),
        std::any_cast<const std::string&>(info->fields[1].value),
        std::any_cast<const std::string&>(info->fields[2].value)};
}

std::any copy_value(const void* value)
{
    return value ? *static_cast<const std::any*>(value) : std::any{};
}

dynamic_struct make_option(const char* name, bool present, const void* value)
{
    struct_fields fields(2);
    fields[0] = {"present", present};
    fields[1] = {"value", present ? copy_value(value) : std::any{}};
    return dynamic_struct(dynamic_struct_data{name, "option", std::move(fields)});
}

template<class value_type>
int new_scalar_option(const char* name, bool present, value_type value,
                      void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        struct_fields fields(2);
        fields[0] = {"present", present};
        fields[1] = {"value", present ? std::any(value) : std::any{}};
        *result = tx_generated::detail::make_handle<std::any>(dynamic_struct(
            dynamic_struct_data{name, "option", std::move(fields)}));
    });
}

template<class value_type>
int scalar_option_value(const void* value, value_type* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        const auto& option = require_sum(value, "option<");
        if (!state(option))
        {
            throw runtime_failure({tx::error_kind::runtime,
                "invalid_state", "空 option 没有值"});
        }
        *result = std::any_cast<value_type>(option->fields[1].value);
    });
}

template<class value_type>
int unpack_scalar_option(const void* source, bool* present,
                         value_type* value) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        const auto& option = require_sum(source, "option<");
        const bool has_value = state(option);
        const value_type unpacked = has_value
            ? std::any_cast<value_type>(option->fields[1].value)
            : value_type{};
        *present = has_value;
        *value = unpacked;
    });
}

dynamic_struct make_result(const char* name, bool success, const void* value)
{
    struct_fields fields(3);
    fields[0] = {"ok", success};
    fields[1] = {"value", success ? copy_value(value) : std::any{}};
    fields[2] = {"error", success ? std::any{} : copy_value(value)};
    return dynamic_struct(dynamic_struct_data{name, "result", std::move(fields)});
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_option_new(const char* type_name, bool present,
                                 const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        if (present && !value)
        {
            throw std::runtime_error("有值 option 缺少元素");
        }
        *result = make_handle<std::any>(make_option(type_name, present, value));
    });
}

extern "C" int txrt_option_new_i64(const char* type_name, bool present,
    std::int64_t value, void** result) noexcept
{
    return new_scalar_option(type_name, present, value, result);
}

extern "C" int txrt_option_new_f64(const char* type_name, bool present,
    double value, void** result) noexcept
{
    return new_scalar_option(type_name, present, value, result);
}

extern "C" int txrt_option_new_bool(const char* type_name, bool present,
    bool value, void** result) noexcept
{
    return new_scalar_option(type_name, present, value, result);
}

extern "C" int txrt_result_new(const char* type_name, bool success,
                                 const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        if (!success)
        {
            (void)read_error(copy_value(value));
        }
        *result = make_handle<std::any>(make_result(type_name, success, value));
    });
}

extern "C" int txrt_result_from_legacy(const char* type_name,
                                         const void* legacy,
                                         void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& old = std::any_cast<const dynamic_struct&>(
            *static_cast<const std::any*>(legacy));
        if (old->fields.size() != 3)
        {
            throw std::runtime_error("旧结果结构不完整");
        }
        struct_fields fields(3);
        const bool success = std::any_cast<bool>(old->fields[0].value);
        fields[0] = {"ok", success};
        fields[1] = {"value", old->fields[1].value};
        fields[2] = {"error", old->fields[2].value};
        if (!success)
        {
            (void)read_error(fields[2].value);
        }
        *result = make_handle<std::any>(dynamic_struct(
            dynamic_struct_data{type_name, "result", std::move(fields)}));
    });
}

extern "C" bool txrt_sum_state(const void* value) noexcept
{
    bool result = false;
    txrt_require_success(invoke_checked([&]
    {
        result = state(std::any_cast<const dynamic_struct&>(
            *static_cast<const std::any*>(value)));
    }));
    return result;
}

extern "C" int txrt_option_value(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& option = require_sum(value, "option<");
        if (!state(option))
        {
            throw runtime_failure({tx::error_kind::runtime,
                "invalid_state", "空 option 没有值"});
        }
        *result = make_handle<std::any>(option->fields[1].value);
    });
}

extern "C" int txrt_option_value_i64(const void* value,
    std::int64_t* result) noexcept
{
    return scalar_option_value(value, result);
}

extern "C" int txrt_option_value_f64(const void* value,
    double* result) noexcept
{
    return scalar_option_value(value, result);
}

extern "C" int txrt_option_value_bool(const void* value,
    bool* result) noexcept
{
    return scalar_option_value(value, result);
}

extern "C" int txrt_option_unpack_i64(const void* source,
    bool* present, std::int64_t* value) noexcept
{
    return unpack_scalar_option(source, present, value);
}

extern "C" int txrt_option_unpack_f64(const void* source,
    bool* present, double* value) noexcept
{
    return unpack_scalar_option(source, present, value);
}

extern "C" int txrt_option_unpack_bool(const void* source,
    bool* present, bool* value) noexcept
{
    return unpack_scalar_option(source, present, value);
}

extern "C" int txrt_option_empty_error() noexcept
{
    return invoke_checked([]
    {
        throw runtime_failure({tx::error_kind::runtime,
            "invalid_state", "空 option 没有值"});
    });
}

extern "C" int txrt_option_value_or(const void* value,
                                      const void* fallback,
                                      void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& option = require_sum(value, "option<");
        *result = make_handle<std::any>(state(option)
            ? option->fields[1].value : copy_value(fallback));
    });
}

extern "C" int txrt_result_value(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& outcome = require_sum(value, "result<");
        if (!state(outcome))
        {
            throw runtime_failure(read_error(outcome->fields[2].value));
        }
        if (result)
        {
            *result = make_handle<std::any>(outcome->fields[1].value);
        }
    });
}

extern "C" int txrt_result_error(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& outcome = require_sum(value, "result<");
        if (state(outcome))
        {
            throw runtime_failure({tx::error_kind::runtime,
                "invalid_state", "成功 result 没有错误"});
        }
        *result = make_handle<std::any>(outcome->fields[2].value);
    });
}
