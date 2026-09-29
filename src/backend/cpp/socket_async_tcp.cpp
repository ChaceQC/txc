#include "backend/cpp/socket_async_loop.hpp"

#include "backend/cpp/socket_abi_helpers.hpp"

#include <algorithm>
#include <any>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace tx_generated::socket_async
{
namespace
{

constexpr std::size_t transfer_chunk = 16 * 1024;
constexpr std::size_t maximum_transfer = 16 * 1024 * 1024;

void check_stream(const std::shared_ptr<socket::state>& value)
{
    if (value->kind != socket::resource_kind::tcp_stream)
    {
        network::fail("invalid_argument", "需要 TCP 流句柄");
    }
}

[[noreturn]] void stream_error(const std::shared_ptr<socket::state>& peer,
    std::string_view action)
{
    const int code = WSAGetLastError();
    if (code == WSAECONNRESET || code == WSAECONNABORTED ||
        code == WSAENOTCONN || code == WSAESHUTDOWN)
    {
        peer->closed = true;
        std::lock_guard lock(peer->mutex);
        peer->native.reset();
    }
    WSASetLastError(code);
    socket::socket_error(action);
}

task_result stream_value(const std::string& name, socket::resource value)
{
    return std::any(socket_abi::resource_value(name.c_str(),
        "tcp_stream", std::move(value)));
}

struct connection_data
{
    socket::address_list addresses{nullptr, freeaddrinfo};
};

} // namespace

std::shared_ptr<operation> make_connect(std::string ip, std::int64_t port,
    std::int64_t timeout_ms, std::string result_name)
{
    auto value = std::make_shared<operation>();
    value->timeout_ms = timeout_ms;
    value->write = true;
    auto connection = std::make_shared<connection_data>();
    value->start = [ip = std::move(ip), port, result_name,
        connection](operation& pending) -> step_result
    {
        socket::check_port(port, false);
        socket::check_timeout(pending.timeout_ms);
        connection->addresses = socket::numeric_addresses(ip, port,
            SOCK_STREAM, IPPROTO_TCP, false);
        for (auto* item = connection->addresses.get(); item;
             item = item->ai_next)
        {
            auto native = socket::create_socket(*item);
            const int started = ::connect(native.get(), item->ai_addr,
                static_cast<int>(item->ai_addrlen));
            if (started == 0)
            {
                return stream_value(result_name, socket::register_socket(
                    socket::resource_kind::tcp_stream, std::move(native)));
            }
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                pending.native = std::make_shared<network::socket_handle>(
                    std::move(native));
                return {};
            }
        }
        socket::socket_error("连接 TCP 目标");
    };
    value->ready = [result_name](operation& pending) -> step_result
    {
        int error = 0;
        int length = sizeof(error);
        if (getsockopt(pending.native->get(), SOL_SOCKET, SO_ERROR,
                reinterpret_cast<char*>(&error), &length) != 0)
        {
            socket::socket_error("连接 TCP 目标");
        }
        if (error != 0)
        {
            WSASetLastError(error);
            socket::socket_error("连接 TCP 目标");
        }
        auto native = std::move(*pending.native);
        pending.native.reset();
        return stream_value(result_name, socket::register_socket(
            socket::resource_kind::tcp_stream, std::move(native)));
    };
    return value;
}

std::shared_ptr<operation> make_accept(std::shared_ptr<socket::state> listener,
    std::int64_t timeout_ms, std::string result_name)
{
    auto value = std::make_shared<operation>();
    value->state = std::move(listener);
    value->timeout_ms = timeout_ms;
    value->start = [](operation& pending) -> step_result
    {
        socket::check_timeout(pending.timeout_ms);
        pending.native = socket::native_socket(pending.state);
        return {};
    };
    value->ready = [result_name](operation& pending) -> step_result
    {
        network::socket_handle accepted(::accept(pending.native->get(),
            nullptr, nullptr));
        if (!accepted.valid())
        {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                return {};
            }
            socket::socket_error("接受 TCP 连接");
        }
        u_long nonblocking = 1;
        if (ioctlsocket(accepted.get(), FIONBIO, &nonblocking) != 0)
        {
            socket::socket_error("设置 TCP 连接非阻塞模式");
        }
        return stream_value(result_name, socket::register_socket(
            socket::resource_kind::tcp_stream, std::move(accepted)));
    };
    return value;
}

std::shared_ptr<operation> make_read(std::shared_ptr<socket::state> peer,
    std::int64_t max_bytes, std::int64_t timeout_ms, std::string result_name)
{
    auto value = std::make_shared<operation>();
    value->state = std::move(peer);
    value->timeout_ms = timeout_ms;
    auto buffer = std::make_shared<std::vector<std::uint8_t>>();
    value->start = [max_bytes, result_name,
        buffer](operation& pending) -> step_result
    {
        check_stream(pending.state);
        socket::check_timeout(pending.timeout_ms);
        if (max_bytes < 1 || max_bytes >
            static_cast<std::int64_t>(maximum_transfer))
        {
            network::fail("invalid_argument", "TCP 读取长度必须为 1～16 MiB");
        }
        if (pending.state->read_closed)
        {
            network::fail("connection_closed", "TCP 读取方向已关闭");
        }
        if (pending.state->remote_eof)
        {
            return task_result(std::any(socket_abi::read_value(
                result_name.c_str(), {{}, true})));
        }
        pending.native = socket::native_socket(pending.state);
        buffer->resize(std::min<std::size_t>(max_bytes, transfer_chunk));
        return {};
    };
    value->ready = [buffer, result_name](operation& pending) -> step_result
    {
        if (pending.state->read_closed)
        {
            network::fail("connection_closed", "TCP 读取方向已关闭");
        }
        if (pending.state->remote_eof)
        {
            return task_result(std::any(socket_abi::read_value(
                result_name.c_str(), {{}, true})));
        }
        const int received = recv(pending.native->get(),
            reinterpret_cast<char*>(buffer->data()),
            static_cast<int>(buffer->size()), 0);
        if (received > 0)
        {
            buffer->resize(static_cast<std::size_t>(received));
            return task_result(std::any(socket_abi::read_value(
                result_name.c_str(), {std::move(*buffer), false})));
        }
        if (received == 0)
        {
            pending.state->remote_eof = true;
            return task_result(std::any(socket_abi::read_value(
                result_name.c_str(), {{}, true})));
        }
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            return {};
        }
        stream_error(pending.state, "读取 TCP 数据");
    };
    return value;
}

std::shared_ptr<operation> make_write(std::shared_ptr<socket::state> peer,
    byte_value bytes, std::int64_t timeout_ms)
{
    auto value = std::make_shared<operation>();
    value->state = std::move(peer);
    value->timeout_ms = timeout_ms;
    value->write = true;
    value->start = [bytes](operation& pending) -> step_result
    {
        check_stream(pending.state);
        socket::check_timeout(pending.timeout_ms);
        if (bytes->size() > maximum_transfer)
        {
            network::fail("size_limit", "TCP 单次写入输入超过 16 MiB");
        }
        if (pending.state->write_closed)
        {
            network::fail("connection_closed", "TCP 写入方向已关闭");
        }
        pending.native = socket::native_socket(pending.state);
        if (bytes->empty())
        {
            return task_result(std::int64_t{0});
        }
        return {};
    };
    value->ready = [bytes](operation& pending) -> step_result
    {
        if (pending.state->write_closed)
        {
            network::fail("connection_closed", "TCP 写入方向已关闭");
        }
        const int sent = send(pending.native->get(),
            reinterpret_cast<const char*>(bytes->data()),
            static_cast<int>(std::min(bytes->size(), transfer_chunk)), 0);
        if (sent > 0)
        {
            return task_result(static_cast<std::int64_t>(sent));
        }
        if (sent == 0)
        {
            network::fail("connection_closed", "TCP 对端已关闭连接");
        }
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            return {};
        }
        stream_error(pending.state, "写入 TCP 数据");
    };
    return value;
}

} // namespace tx_generated::socket_async
