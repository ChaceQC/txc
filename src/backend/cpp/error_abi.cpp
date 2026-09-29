#include "backend/cpp/error_abi.hpp"
#include "backend/cpp/error_result.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"

#include <any>
#include <cstdio>
#include <cstring>
#include <string>

namespace tx_generated::detail
{

void set_error(runtime_context& context, tx::error_kind kind,
               const char* code, const char* message) noexcept
{
    try
    {
        context.last_error_stack = capture_stack(context);
    }
    catch (...)
    {
        context.last_error_stack.clear();
    }
    context.last_error_kind = kind;
    std::snprintf(context.last_error_code, sizeof(context.last_error_code), "%s", code);
    std::snprintf(context.last_error, sizeof(context.last_error), "%s", message);
}

void set_error(tx::error_kind kind, const char* code, const char* message) noexcept
{
    set_error(current_runtime_context(), kind, code, message);
}

error_cleanup_guard::error_cleanup_guard() noexcept
    : context_(current_runtime_context()), kind_(context_.last_error_kind)
{
    if (kind_ != tx::error_kind::none)
    {
        std::memcpy(code_, context_.last_error_code, sizeof(code_));
        std::memcpy(message_, context_.last_error, sizeof(message_));
        stack_ = std::move(context_.last_error_stack);
        context_.last_error_kind = tx::error_kind::none;
    }
}

error_cleanup_guard::~error_cleanup_guard()
{
    if (kind_ != tx::error_kind::none)
    {
        context_.last_error_kind = kind_;
        std::memcpy(context_.last_error_code, code_, sizeof(code_));
        std::memcpy(context_.last_error, message_, sizeof(message_));
        context_.last_error_stack = std::move(stack_);
    }
}

dynamic_struct make_error_value(const char* type_name, const error_info& error)
{
    struct_fields fields(3);
    fields[0] = {"kind", std::string(error_kind_name(error.kind))};
    fields[1] = {"code", error.code};
    fields[2] = {"message", error.message};
    return dynamic_struct(dynamic_struct_data{type_name, "error_info", std::move(fields)});
}

} // namespace tx_generated::detail

extern "C" int txrt_error_status() noexcept
{
    return static_cast<int>(tx_generated::detail::current_runtime_context().last_error_kind);
}

extern "C" void txrt_error_propagation(bool enabled) noexcept
{
    tx_generated::detail::current_runtime_context().propagate_errors = enabled;
}

extern "C" int txrt_error_take(const char* type_name, void** result) noexcept
{
    using namespace tx_generated::detail;
    auto& context = current_runtime_context();
    // 清除发生在结果构造之前，构造失败将成为交给外层处理器的新错误。
    const auto kind = context.last_error_kind;
    char code[64];
    char message[256];
    std::memcpy(code, context.last_error_code, sizeof(code));
    std::memcpy(message, context.last_error, sizeof(message));
    context.last_error_kind = tx::error_kind::none;
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(make_error_value(type_name, {kind, code, message}));
    });
}

extern "C" int txrt_error_fail_io(const void* code,
                                   const void* message) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        throw tx_generated::runtime_failure({
            tx::error_kind::io,
            tx_generated::detail::text_value(code),
            tx_generated::detail::text_value(message)});
    });
}

extern "C" void txrt_stack_error_location(void* raw_context, const char* file, std::size_t line,
    std::size_t column) noexcept
{
    using namespace tx_generated::detail;
    auto& context = *static_cast<runtime_context*>(raw_context);
    std::size_t depth = 0;
    for (auto* frame = context.active_frame; frame; frame = frame->parent)
    {
        ++depth;
    }
    // 报错时的活动栈比错误快照短：已返回的被调函数仍留在快照末尾。
    if (depth == 0 || depth > context.last_error_stack.size())
    {
        return;
    }
    auto& frame = context.last_error_stack[depth - 1];
    frame.file = file ? file : "";
    frame.line = line;
    frame.column = column;
}

extern "C" int txrt_error_stack_trace(void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        std::string text;
        for (const auto& frame : tx_generated::detail::current_runtime_context().last_error_stack)
        {
            if (!text.empty())
            {
                text += '\n';
            }
            text += frame.function;
            text += " (";
            text += frame.file;
            text += ":" + std::to_string(frame.line) + ":" +
                std::to_string(frame.column) + ")";
        }
        *result = tx_generated::detail::make_handle<std::string>(std::move(text));
    });
}
