#include "backend/cpp/socket_abi_helpers.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/task_executor.hpp"
#include "backend/cpp/task_runtime_internal.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/cancellation.hpp"
#include "stdlib/error.hpp"

#include <any>
#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace
{

using tx_generated::socket::cancellation_probe;
using tx_generated::task_result;
using work_function = std::function<task_result(const cancellation_probe&)>;

std::shared_ptr<tx_generated::cancellation_state> token_state(const void* value)
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
               const void* token_value, work_function work)
{
    auto scope = tx_generated::current_task_scope();
    if (!scope)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_state", "异步网络操作需要活动的 task.scope"});
    }
    auto token = token_state(token_value);
    auto child = tx_generated::reserve_task(scope);
    void* handle = nullptr;
    try
    {
        handle = tx_generated::detail::make_handle<std::any>(
            tx_generated::task_handle{child, kind, task_name});
        tx_generated::enqueue_task([scope, token, child,
            work = std::move(work)]() mutable
        {
            const cancellation_probe probe = [scope, token]
            {
                if (tx_generated::task_cancelled(*scope))
                {
                    throw tx_generated::runtime_failure({tx::error_kind::cancelled,
                        "cancelled", "网络任务作用域已取消"});
                }
                std::lock_guard lock(token->mutex);
                if (token->cancelled)
                {
                    throw tx_generated::runtime_failure({tx::error_kind::cancelled,
                        "cancelled", "网络操作已取消"});
                }
                if (token->deadline && std::chrono::steady_clock::now() >=
                    *token->deadline)
                {
                    throw tx_generated::runtime_failure({tx::error_kind::cancelled,
                        "deadline_exceeded", "网络操作截止时间已到"});
                }
            };
            try
            {
                probe();
                tx_generated::complete_task(child, work(probe), {});
            }
            catch (const tx_generated::runtime_failure& failure)
            {
                const auto& error = failure.error();
                tx_generated::complete_task(child, {}, {error.kind,
                    error.code, error.message, {}});
            }
            catch (const std::exception& failure)
            {
                tx_generated::complete_task(child, {}, {tx::error_kind::io,
                    "operation_failed", failure.what(), {}});
            }
            catch (...)
            {
                tx_generated::complete_task(child, {}, {tx::error_kind::io,
                    "operation_failed", "异步网络操作失败", {}});
            }
        });
    }
    catch (...)
    {
        if (handle)
        {
            tx_generated::detail::destroy_handle(
                static_cast<std::any*>(handle));
        }
        tx_generated::discard_task(child);
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
        auto address = *static_cast<const std::string*>(ip);
        *result = schedule(5, task_name, token,
            [address = std::move(address), port, timeout_ms,
                name = std::string(stream_name)](const cancellation_probe& probe)
                -> task_result
            {
                return std::any(tx_generated::socket_abi::resource_value(
                    name.c_str(), "tcp_stream",
                    tx_generated::socket::connect_tcp(address, port,
                        timeout_ms, probe)));
            });
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
            [listener = std::move(listener), timeout_ms,
                name = std::string(stream_name)](const cancellation_probe& probe)
                -> task_result
            {
                return std::any(tx_generated::socket_abi::resource_value(
                    name.c_str(), "tcp_stream",
                    tx_generated::socket::accept_tcp(listener, timeout_ms,
                        probe)));
            });
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
            [state = std::move(state), max_bytes, timeout_ms,
                name = std::string(result_name)](const cancellation_probe& probe)
                -> task_result
            {
                return std::any(tx_generated::socket_abi::read_value(name.c_str(),
                    tx_generated::socket::read(state, max_bytes, timeout_ms, probe)));
            });
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
            [state = std::move(state), bytes, timeout_ms](
                const cancellation_probe& probe) -> task_result
            {
                const std::string_view view(bytes->empty() ? "" :
                    reinterpret_cast<const char*>(bytes->data()), bytes->size());
                return tx_generated::socket::write(state, view,
                    timeout_ms, probe);
            });
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
        auto address = *static_cast<const std::string*>(ip);
        const auto bytes = tx_generated::bytes_of(
            *static_cast<const std::any*>(data));
        *result = schedule(1, task_name, token,
            [state = std::move(state), address = std::move(address), port,
                bytes, timeout_ms](const cancellation_probe& probe) -> task_result
            {
                const std::string_view view(bytes->empty() ? "" :
                    reinterpret_cast<const char*>(bytes->data()), bytes->size());
                return tx_generated::socket::send_to(state, address,
                    port, view, timeout_ms, probe);
            });
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
            [state = std::move(state), max_bytes, timeout_ms,
                name = std::string(result_name)](const cancellation_probe& probe)
                -> task_result
            {
                return std::any(tx_generated::socket_abi::datagram_value(
                    name.c_str(), tx_generated::socket::receive_from(
                        state, max_bytes, timeout_ms, probe)));
            });
    }, tx::error_kind::io);
}
