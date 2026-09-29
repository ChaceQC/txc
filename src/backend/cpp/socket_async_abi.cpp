#include "backend/cpp/socket_async_loop.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/socket_abi_helpers.hpp"
#include "stdlib/bytes.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace
{

std::shared_ptr<tx_generated::cancellation_state> token_state(
    const void* value)
{
    const auto& token = std::any_cast<const tx_generated::cancel_token&>(
        *static_cast<const std::any*>(value));
    if (!token.state)
    {
        tx_generated::network::fail("invalid_argument", "网络取消令牌无效");
    }
    return token.state;
}

void* schedule(std::int64_t kind, const char* task_name,
    const void* token_value,
    std::shared_ptr<tx_generated::socket_async::operation> operation)
{
    auto scope = tx_generated::current_task_scope();
    if (!scope)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_state", "异步网络操作需要活动的 task.scope"});
    }
    operation->scope = std::move(scope);
    operation->token = token_state(token_value);
    operation->child = tx_generated::reserve_task(operation->scope);
    void* handle = nullptr;
    try
    {
        handle = tx_generated::detail::make_handle<std::any>(
            tx_generated::task_handle{operation->child, kind, task_name});
        tx_generated::socket_async::submit(operation);
    }
    catch (...)
    {
        if (handle)
        {
            tx_generated::detail::destroy_handle(
                static_cast<std::any*>(handle));
        }
        tx_generated::discard_task(operation->child);
        throw;
    }
    return handle;
}

} // namespace

extern "C" int txrt_socket_connect_async(const void* ip,
    std::int64_t port, std::int64_t timeout_ms, const void* token,
    const char* task_name, const char* stream_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = schedule(5, task_name, token,
            tx_generated::socket_async::make_connect(
                tx_generated::detail::text_value(ip), port, timeout_ms,
                stream_name));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_accept_async(const void* server,
    std::int64_t timeout_ms, const void* token,
    const char* task_name, const char* stream_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto listener = tx_generated::socket_abi::state_at(server,
            tx_generated::socket::resource_kind::tcp_listener);
        *result = schedule(5, task_name, token,
            tx_generated::socket_async::make_accept(std::move(listener),
                timeout_ms, stream_name));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_read_async(const void* peer,
    std::int64_t max_bytes, std::int64_t timeout_ms, const void* token,
    const char* task_name, const char* result_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto state = tx_generated::socket_abi::state_at(peer,
            tx_generated::socket::resource_kind::tcp_stream);
        *result = schedule(5, task_name, token,
            tx_generated::socket_async::make_read(std::move(state),
                max_bytes, timeout_ms, result_name));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_write_async(const void* peer, const void* data,
    std::int64_t timeout_ms, const void* token, const char* task_name,
    void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto state = tx_generated::socket_abi::state_at(peer,
            tx_generated::socket::resource_kind::tcp_stream);
        const auto bytes = tx_generated::bytes_of(
            *static_cast<const std::any*>(data));
        *result = schedule(1, task_name, token,
            tx_generated::socket_async::make_write(std::move(state),
                bytes, timeout_ms));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_send_to_async(const void* endpoint,
    const void* ip, std::int64_t port, const void* data,
    std::int64_t timeout_ms, const void* token, const char* task_name,
    void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto state = tx_generated::socket_abi::state_at(endpoint,
            tx_generated::socket::resource_kind::udp_socket);
        const auto bytes = tx_generated::bytes_of(
            *static_cast<const std::any*>(data));
        *result = schedule(1, task_name, token,
            tx_generated::socket_async::make_send_to(std::move(state),
                tx_generated::detail::text_value(ip), port, bytes,
                timeout_ms));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_receive_from_async(const void* endpoint,
    std::int64_t max_bytes, std::int64_t timeout_ms, const void* token,
    const char* task_name, const char* result_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        auto state = tx_generated::socket_abi::state_at(endpoint,
            tx_generated::socket::resource_kind::udp_socket);
        *result = schedule(5, task_name, token,
            tx_generated::socket_async::make_receive_from(std::move(state),
                max_bytes, timeout_ms, result_name));
    }, tx::error_kind::io);
}
