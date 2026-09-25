#include "backend/cpp/json_abi.hpp"
#include "backend/cpp/error_result.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/json.hpp"

#include <any>
#include <string>
#include <utility>

namespace
{

const std::string& text_value(const void* value)
{
    return *static_cast<const std::string*>(value);
}

const std::any& any_value(const void* value)
{
    return *static_cast<const std::any*>(value);
}

const tx_generated::tx_dict& object_value(const void* value)
{
    return std::any_cast<const tx_generated::tx_dict&>(any_value(value));
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_json_parse(const void* text, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::json_parse(text_value(text)));
    });
}

extern "C" int txrt_json_parse_object(const void* text, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::json_parse_object(
            text_value(text)));
    });
}

extern "C" int txrt_json_try_parse(const void* text, const char* result_type,
    const char* error_type, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::detail::make_result_value(
            result_type, error_type, tx_generated::json_try_parse(text_value(text))));
    });
}

extern "C" int txrt_json_stringify(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::json_stringify(
            any_value(value)));
    });
}

extern "C" int txrt_json_stringify_pretty(const void* value,
    std::int64_t indent, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::json_stringify_pretty(
            any_value(value), indent));
    });
}

extern "C" int txrt_json_contains(const void* object, const void* key,
    bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::json_contains(object_value(object), text_value(key));
    });
}

extern "C" int txrt_json_get(const void* object, const void* key,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::json_get(
            object_value(object), text_value(key)));
    });
}

extern "C" int txrt_json_get_int(const void* object, const void* key,
    std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::json_get_int(object_value(object), text_value(key));
    });
}

extern "C" int txrt_json_get_float(const void* object, const void* key,
    double* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::json_get_float(object_value(object), text_value(key));
    });
}

extern "C" int txrt_json_get_bool(const void* object, const void* key,
    bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::json_get_bool(object_value(object), text_value(key));
    });
}

extern "C" int txrt_json_get_str(const void* object, const void* key,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::json_get_str(
            object_value(object), text_value(key)));
    });
}

extern "C" int txrt_json_get_array(const void* object, const void* key,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::json_get_array(
            object_value(object), text_value(key)));
    });
}

extern "C" int txrt_json_get_object(const void* object, const void* key,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::json_get_object(
            object_value(object), text_value(key)));
    });
}
