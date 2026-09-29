#include "backend/cpp/socket_abi_helpers.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/bytes.hpp"

#include <any>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace tx_generated::socket_abi
{

std::int64_t resource_id(const void* value)
{
    const auto& object = std::any_cast<const dynamic_struct&>(
        *static_cast<const std::any*>(value));
    if (object->fields.size() != 1)
    {
        network::fail("invalid_argument", "网络句柄字段无效");
    }
    return std::any_cast<std::int64_t>(object->fields[0].value);
}

std::shared_ptr<socket::state> state_at(const void* value,
                                        socket::resource_kind kind)
{
    return socket::get(resource_id(value), kind);
}

std::string_view bytes_at(const void* value)
{
    const auto& bytes = bytes_of(*static_cast<const std::any*>(value));
    return {bytes->empty() ? "" :
        reinterpret_cast<const char*>(bytes->data()), bytes->size()};
}

dynamic_struct resource_value(const char* type_name,
                              const char* display_name,
                              socket::resource value)
{
    struct_fields fields(1);
    fields[0] = {"id", value.id};
    std::shared_ptr<const void> owner = std::move(value.value);
    return dynamic_struct(dynamic_struct_data{type_name, display_name,
        std::move(fields), std::move(owner)});
}

dynamic_struct read_value(const char* type_name, socket::read_result value)
{
    struct_fields fields(2);
    fields[0] = {"data", make_bytes(std::move(value.data))};
    fields[1] = {"eof", value.eof};
    return dynamic_struct(dynamic_struct_data{type_name, "read_result",
        std::move(fields)});
}

dynamic_struct datagram_value(const char* type_name, socket::datagram value)
{
    struct_fields fields(4);
    fields[0] = {"data", make_bytes(std::move(value.data))};
    fields[1] = {"host", std::move(value.host)};
    fields[2] = {"port", value.port};
    fields[3] = {"truncated", value.truncated};
    return dynamic_struct(dynamic_struct_data{type_name, "datagram",
        std::move(fields)});
}

} // namespace tx_generated::socket_abi

namespace
{

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;
using tx_generated::socket::resource_kind;

const std::string& text_at(const void* value)
{
    return tx_generated::detail::text_value(value);
}

} // namespace

extern "C" int txrt_socket_listen_tcp(const void* ip, std::int64_t port,
    std::int64_t backlog, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::socket_abi::resource_value(
            type_name, "tcp_listener", tx_generated::socket::listen_tcp(
                text_at(ip), port, backlog)));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_listener_port(const void* server,
    std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::socket::local_port(
            tx_generated::socket_abi::state_at(server,
                resource_kind::tcp_listener));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_connect_tcp(const void* ip, std::int64_t port,
    std::int64_t timeout_ms, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::socket_abi::resource_value(
            type_name, "tcp_stream", tx_generated::socket::connect_tcp(
                text_at(ip), port, timeout_ms)));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_accept_tcp(const void* server,
    std::int64_t timeout_ms, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::socket_abi::resource_value(
            type_name, "tcp_stream", tx_generated::socket::accept_tcp(
                tx_generated::socket_abi::state_at(server,
                    resource_kind::tcp_listener), timeout_ms)));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_read(const void* peer,
    std::int64_t max_bytes, std::int64_t timeout_ms,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::socket_abi::read_value(
            type_name, tx_generated::socket::read(
                tx_generated::socket_abi::state_at(peer,
                    resource_kind::tcp_stream), max_bytes, timeout_ms)));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_write(const void* peer, const void* data,
    std::int64_t timeout_ms, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::socket::write(
            tx_generated::socket_abi::state_at(peer,
                resource_kind::tcp_stream),
            tx_generated::socket_abi::bytes_at(data), timeout_ms);
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_shutdown_read(const void* peer) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::socket::shutdown_read(tx_generated::socket_abi::state_at(
            peer, resource_kind::tcp_stream));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_shutdown_write(const void* peer) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::socket::shutdown_write(tx_generated::socket_abi::state_at(
            peer, resource_kind::tcp_stream));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_bind_udp(const void* ip, std::int64_t port,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::socket_abi::resource_value(
            type_name, "udp_socket", tx_generated::socket::bind_udp(
                text_at(ip), port)));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_udp_port(const void* endpoint,
    std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::socket::local_port(
            tx_generated::socket_abi::state_at(endpoint,
                resource_kind::udp_socket));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_send_to(const void* endpoint, const void* ip,
    std::int64_t port, const void* data, std::int64_t timeout_ms,
    std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::socket::send_to(
            tx_generated::socket_abi::state_at(endpoint,
                resource_kind::udp_socket), text_at(ip), port,
            tx_generated::socket_abi::bytes_at(data), timeout_ms);
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_receive_from(const void* endpoint,
    std::int64_t max_bytes, std::int64_t timeout_ms,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::socket_abi::datagram_value(
            type_name, tx_generated::socket::receive_from(
                tx_generated::socket_abi::state_at(endpoint,
                    resource_kind::udp_socket), max_bytes, timeout_ms)));
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_close_tcp_listener(const void* server) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::socket::close(tx_generated::socket_abi::resource_id(server),
            resource_kind::tcp_listener);
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_close_tcp_stream(const void* peer) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::socket::close(tx_generated::socket_abi::resource_id(peer),
            resource_kind::tcp_stream);
    }, tx::error_kind::io);
}

extern "C" int txrt_socket_close_udp_socket(const void* endpoint) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::socket::close(tx_generated::socket_abi::resource_id(endpoint),
            resource_kind::udp_socket);
    }, tx::error_kind::io);
}
