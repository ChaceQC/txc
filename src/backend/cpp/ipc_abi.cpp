#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/ipc.hpp"

#include <any>
#include <cstdint>
#include <string>

namespace
{

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

template<class value_type>
const value_type& input(const void* value)
{
    return std::any_cast<const value_type&>(
        *static_cast<const std::any*>(value));
}

std::shared_ptr<tx_generated::cancellation_state> token_of(
    const void* value)
{
    const auto& token = input<tx_generated::cancel_token>(value);
    if (!token.state)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_state", "IPC 取消令牌已失效"});
    }
    return token.state;
}

tx_generated::dynamic_struct make_message(
    const tx_generated::ipc_message& message, const char* type_name)
{
    tx_generated::struct_fields fields(6);
    fields[0] = {"state", message.state};
    fields[1] = {"version", message.version};
    fields[2] = {"session", message.session};
    fields[3] = {"value", message.value};
    fields[4] = {"received", message.received};
    fields[5] = {"error_code", message.error_code};
    return tx_generated::dynamic_struct(
        tx_generated::dynamic_struct_data{type_name, "message",
            std::move(fields)});
}

tx_generated::dynamic_struct make_send_result(
    const tx_generated::ipc_send_result& value, const char* type_name)
{
    tx_generated::struct_fields fields(3);
    fields[0] = {"state", value.state};
    fields[1] = {"written", value.written};
    fields[2] = {"error_code", value.error_code};
    return tx_generated::dynamic_struct(
        tx_generated::dynamic_struct_data{type_name, "send_result",
            std::move(fields)});
}

} // namespace

extern "C" int txrt_ipc_listen(const void* name,
    std::int64_t max_bytes, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::ipc_listen(
            *static_cast<const std::string*>(name), max_bytes));
    });
}

extern "C" int txrt_ipc_accept(const void* listener,
    std::int64_t timeout_ms, const void* token, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::ipc_accept(
            input<tx_generated::ipc_listener>(listener), timeout_ms,
            token_of(token)));
    });
}

extern "C" int txrt_ipc_connect(const void* name,
    std::int64_t max_bytes, std::int64_t timeout_ms,
    const void* token, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::ipc_connect(
            *static_cast<const std::string*>(name), max_bytes,
            timeout_ms, token_of(token)));
    });
}

extern "C" int txrt_ipc_from_process_pipes(const void* reader,
    const void* writer, std::int64_t max_bytes, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::ipc_from_process_pipes(
            input<tx_generated::process_pipe>(reader),
            input<tx_generated::process_pipe>(writer), max_bytes));
    });
}

extern "C" int txrt_ipc_send(const void* stream, std::int64_t version,
    const void* value, std::int64_t timeout_ms, const void* token,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto sent = tx_generated::ipc_send(
            input<tx_generated::ipc_stream>(stream), version,
            *static_cast<const std::any*>(value), timeout_ms,
            token_of(token));
        *result = make_handle<std::any>(
            make_send_result(sent, type_name));
    });
}

extern "C" int txrt_ipc_recv(const void* stream,
    std::int64_t timeout_ms, const void* token,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto message = tx_generated::ipc_receive(
            input<tx_generated::ipc_stream>(stream), timeout_ms,
            token_of(token));
        *result = make_handle<std::any>(
            make_message(message, type_name));
    });
}

extern "C" int txrt_ipc_close(const void* stream,
    bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::ipc_close(
            input<tx_generated::ipc_stream>(stream));
    });
}

extern "C" int txrt_ipc_close_listener(const void* listener,
    bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::ipc_close_listener(
            input<tx_generated::ipc_listener>(listener));
    });
}
