#include "stdlib/socket.hpp"

#include <algorithm>
#include <chrono>
#include <string>

namespace tx_generated::socket
{
namespace
{

constexpr std::size_t transfer_chunk = 16 * 1024;
constexpr std::size_t maximum_transfer = 16 * 1024 * 1024;

void check_stream(const std::shared_ptr<state>& value)
{
    if (value->kind != resource_kind::tcp_stream)
    {
        network::fail("invalid_argument", "需要 TCP 流句柄");
    }
}

[[noreturn]] void stream_error(const std::shared_ptr<state>& peer,
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
    socket_error(action);
}

} // namespace

resource listen_tcp(std::string_view ip, std::int64_t port,
                    std::int64_t backlog)
{
    check_port(port, true);
    if (backlog < 1 || backlog > 128)
    {
        network::fail("invalid_argument", "TCP backlog 必须为 1～128");
    }
    auto addresses = numeric_addresses(ip, port, SOCK_STREAM,
        IPPROTO_TCP, true);
    for (auto* item = addresses.get(); item; item = item->ai_next)
    {
        auto native = create_socket(*item);
        if (network::set_socket_exclusive(native.get()) != 0)
        {
            continue;
        }
        if (ip.empty() && item->ai_family == AF_INET6)
        {
            const DWORD dual_stack = 0;
            network::set_socket_option(native.get(), IPPROTO_IPV6, IPV6_V6ONLY,
                reinterpret_cast<const char*>(&dual_stack),
                sizeof(dual_stack));
        }
        if (bind(native.get(), item->ai_addr,
                static_cast<int>(item->ai_addrlen)) == 0 &&
            ::listen(native.get(), static_cast<int>(backlog)) == 0)
        {
            return register_socket(resource_kind::tcp_listener,
                std::move(native));
        }
    }
    socket_error("绑定 TCP 监听地址");
}

resource connect_tcp(std::string_view ip, std::int64_t port,
                     std::int64_t timeout_ms,
                     const cancellation_probe& cancellation)
{
    check_port(port, false);
    check_timeout(timeout_ms);
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    auto addresses = numeric_addresses(ip, port, SOCK_STREAM,
        IPPROTO_TCP, false);
    for (auto* item = addresses.get(); item; item = item->ai_next)
    {
        auto native = create_socket(*item);
        const int started = ::connect(native.get(), item->ai_addr,
            static_cast<int>(item->ai_addrlen));
        if (started != 0 && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            continue;
        }
        if (started != 0)
        {
            auto pending = std::make_shared<state>();
            pending->kind = resource_kind::tcp_stream;
            pending->native = std::make_shared<network::socket_handle>(
                std::move(native));
            wait_ready(pending, pending->native->get(), true, deadline,
                cancellation);
            int error = 0;
            int length = sizeof(error);
            if (network::get_socket_option(pending->native->get(), SOL_SOCKET, SO_ERROR,
                    reinterpret_cast<char*>(&error), &length) != 0)
            {
                socket_error("连接 TCP 目标");
            }
            if (error != 0)
            {
                WSASetLastError(error);
                socket_error("连接 TCP 目标");
            }
            native = std::move(*pending->native);
        }
        return register_socket(resource_kind::tcp_stream, std::move(native));
    }
    socket_error("连接 TCP 目标");
}

resource accept_tcp(const std::shared_ptr<state>& listener,
                    std::int64_t timeout_ms,
                    const cancellation_probe& cancellation)
{
    check_timeout(timeout_ms);
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    auto native = native_socket(listener);
    while (true)
    {
        wait_ready(listener, native->get(), false, deadline, cancellation);
        network::socket_handle accepted(network::accept_native_socket(native->get()));
        if (accepted.valid())
        {
            u_long nonblocking = 1;
            if (ioctlsocket(accepted.get(), FIONBIO, &nonblocking) != 0)
            {
                socket_error("设置 TCP 连接非阻塞模式");
            }
            return register_socket(resource_kind::tcp_stream,
                std::move(accepted));
        }
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            socket_error("接受 TCP 连接");
        }
    }
}

read_result read(const std::shared_ptr<state>& peer,
                 std::int64_t max_bytes, std::int64_t timeout_ms,
                 const cancellation_probe& cancellation)
{
    check_stream(peer);
    check_timeout(timeout_ms);
    if (max_bytes < 1 || max_bytes > static_cast<std::int64_t>(maximum_transfer))
    {
        network::fail("invalid_argument", "TCP 读取长度必须为 1～16 MiB");
    }
    std::lock_guard io_lock(peer->read_mutex);
    if (peer->read_closed)
    {
        network::fail("connection_closed", "TCP 读取方向已关闭");
    }
    if (peer->remote_eof)
    {
        return {{}, true};
    }
    auto native = native_socket(peer);
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    read_result result;
    result.data.resize(std::min<std::size_t>(max_bytes, transfer_chunk));
    while (true)
    {
        wait_ready(peer, native->get(), false, deadline, cancellation);
        const int received = network::socket_receive(native->get(),
            reinterpret_cast<char*>(result.data.data()),
            static_cast<int>(result.data.size()), 0);
        if (received > 0)
        {
            result.data.resize(static_cast<std::size_t>(received));
            return result;
        }
        if (received == 0)
        {
            result.data.clear();
            result.eof = true;
            peer->remote_eof = true;
            return result;
        }
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            stream_error(peer, "读取 TCP 数据");
        }
    }
}

std::int64_t write(const std::shared_ptr<state>& peer,
                   std::string_view data, std::int64_t timeout_ms,
                   const cancellation_probe& cancellation)
{
    check_stream(peer);
    check_timeout(timeout_ms);
    if (data.size() > maximum_transfer)
    {
        network::fail("size_limit", "TCP 单次写入输入超过 16 MiB");
    }
    std::lock_guard io_lock(peer->write_mutex);
    if (peer->write_closed)
    {
        network::fail("connection_closed", "TCP 写入方向已关闭");
    }
    auto native = native_socket(peer);
    if (data.empty())
    {
        return 0;
    }
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    while (true)
    {
        wait_ready(peer, native->get(), true, deadline, cancellation);
        const int sent = network::socket_send(native->get(), data.data(),
            static_cast<int>(std::min(data.size(), transfer_chunk)), 0);
        if (sent > 0)
        {
            return sent;
        }
        if (sent == 0)
        {
            network::fail("connection_closed", "TCP 对端已关闭连接");
        }
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            stream_error(peer, "写入 TCP 数据");
        }
    }
}

void shutdown_read(const std::shared_ptr<state>& peer)
{
    check_stream(peer);
    std::lock_guard io_lock(peer->read_mutex);
    if (peer->read_closed.exchange(true))
    {
        return;
    }
    auto native = native_socket(peer);
    if (shutdown(native->get(), SD_RECEIVE) != 0 &&
        WSAGetLastError() != WSAENOTCONN)
    {
        socket_error("半关闭 TCP 读取方向");
    }
}

void shutdown_write(const std::shared_ptr<state>& peer)
{
    check_stream(peer);
    std::lock_guard io_lock(peer->write_mutex);
    if (peer->write_closed.exchange(true))
    {
        return;
    }
    auto native = native_socket(peer);
    if (shutdown(native->get(), SD_SEND) != 0 &&
        WSAGetLastError() != WSAENOTCONN)
    {
        socket_error("半关闭 TCP 写入方向");
    }
}

} // namespace tx_generated::socket
