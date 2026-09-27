#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/json.hpp"
#include "stdlib/stdlib.hpp"

#include <any>
#include <cstdint>
#include <string>
#include <vector>

namespace
{

using tx_generated::detail::invoke_checked;
using tx_generated::detail::source_frame;

std::string location_text(const source_frame& frame)
{
    return std::string(frame.file) + ":" + std::to_string(frame.line) + ":" +
           std::to_string(frame.column);
}

std::string stack_text(const std::vector<source_frame>& stack)
{
    std::string result;
    for (const auto& frame : stack)
    {
        if (!result.empty())
        {
            result += '\n';
        }
        result += frame.function;
        result += " (" + location_text(frame) + ")";
    }
    return result;
}

void return_text(std::string value, void** result)
{
    *result = tx_generated::detail::make_handle<std::string>(std::move(value));
}

void print_value(const char* type, const std::string& value)
{
    const auto* frame = tx_generated::detail::current_runtime_context().active_frame;
    const auto position = frame ? location_text(frame->source)
        : std::string("<unknown>:0:0");
    tx_generated::tx_fn_write_error("调试 [" + std::string(type) + "] " +
        position + " = " + value + "\n");
}

} // namespace

extern "C" int txrt_debug_stack_trace(void** result) noexcept
{
    return invoke_checked([&]
    {
        return_text(stack_text(tx_generated::detail::capture_stack(
            tx_generated::detail::current_runtime_context())), result);
    });
}

extern "C" int txrt_debug_last_error_stack(void** result) noexcept
{
    return invoke_checked([&]
    {
        return_text(stack_text(tx_generated::detail::current_runtime_context().last_error_stack), result);
    });
}

extern "C" int txrt_debug_location(void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto* frame = tx_generated::detail::current_runtime_context().active_frame;
        return_text(frame ? location_text(frame->source) : "", result);
    });
}

extern "C" int txrt_debug_dump_i64(std::int64_t value) noexcept
{
    return invoke_checked([&]
    {
        print_value("int", tx_generated::tx_int_to_string(value));
    });
}

extern "C" int txrt_debug_dump_f64(double value) noexcept
{
    return invoke_checked([&]
    {
        print_value("float", tx_generated::tx_float_to_string(value));
    });
}

extern "C" int txrt_debug_dump_bool(bool value) noexcept
{
    return invoke_checked([&]
    {
        print_value("bool", tx_generated::tx_bool_to_string(value));
    });
}

extern "C" int txrt_debug_dump_str(const void* value) noexcept
{
    return invoke_checked([&]
    {
        const auto& str = *static_cast<const std::string*>(value);
        print_value("str", tx_generated::json_stringify(std::any(str)));
    });
}
