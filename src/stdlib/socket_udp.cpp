#include "stdlib/socket.hpp"

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

namespace tx_generated::socket
{
namespace
{

constexpr std::size_t maximum_datagram = 65507;

void check_endpoint(const std::shared_ptr<state>& value)
{
    if (value->kind != resource_kind::udp_socket)
    {
        network::fail("invalid_argument", "需要 UDP 句柄");
    }
}

} // namespace

resource bind_udp(std::string_view ip, std::int64_t port)
{
    check_port(port, true);
    auto addresses = numeric_addresses(ip, port, SOCK_DGRAM,
        IPPROTO_UDP, true);
    for (auto* item = addresses.get(); item; item = item->ai_next)
    {
        auto native = create_socket(*item);
        if (ip.empty() && item->ai_family == AF_INET6)
        {
            const DWORD dual_stack = 0;
            network::set_socket_option(native.get(), IPPROTO_IPV6, IPV6_V6ONLY,
                reinterpret_cast<const char*>(&dual_stack),
                sizeof(dual_stack));
        }
        if (bind(native.get(), item->ai_addr,
                static_cast<int>(item->ai_addrlen)) == 0)
        {
            return register_socket(resource_kind::udp_socket,
                std::move(native));
        }
    }
    socket_error("绑定 UDP 地址");
}

std::int64_t send_to(const std::shared_ptr<state>& endpoint,
                     std::string_view ip, std::int64_t port,
                     std::string_view data, std::int64_t timeout_ms,
                     const cancellation_probe& cancellation)
{
    check_endpoint(endpoint);
    check_port(port, false);
    check_timeout(timeout_ms);
    if (data.size() > maximum_datagram)
    {
        network::fail("size_limit", "UDP 报文超过 65507 字节");
    }
    auto addresses = numeric_addresses(ip, port, SOCK_DGRAM,
        IPPROTO_UDP, false);
    std::lock_guard io_lock(endpoint->write_mutex);
    auto native = native_socket(endpoint);
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    for (auto* item = addresses.get(); item; item = item->ai_next)
    {
        while (true)
        {
            wait_ready(endpoint, native->get(), true, deadline, cancellation);
            const int sent = sendto(native->get(), data.data(),
                static_cast<int>(data.size()), 0, item->ai_addr,
                static_cast<int>(item->ai_addrlen));
            if (sent >= 0)
            {
                if (static_cast<std::size_t>(sent) != data.size())
                {
                    network::fail("short_write", "UDP 报文未完整发送");
                }
                return sent;
            }
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                socket_error("发送 UDP 报文");
            }
        }
    }
    network::fail("invalid_argument", "UDP 目的地址无效");
}

datagram receive_from(const std::shared_ptr<state>& endpoint,
                      std::int64_t max_bytes, std::int64_t timeout_ms,
                      const cancellation_probe& cancellation)
{
    check_endpoint(endpoint);
    check_timeout(timeout_ms);
    if (max_bytes < 0 || max_bytes > static_cast<std::int64_t>(maximum_datagram))
    {
        network::fail("invalid_argument", "UDP 接收上限必须为 0～65507 字节");
    }
    std::lock_guard io_lock(endpoint->read_mutex);
    auto native = native_socket(endpoint);
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    std::vector<std::uint8_t> buffer(65536);
    while (true)
    {
        wait_ready(endpoint, native->get(), false, deadline, cancellation);
        sockaddr_storage source{};
        network::socket_length source_length = sizeof(source);
        const int received = recvfrom(native->get(),
            reinterpret_cast<char*>(buffer.data()),
            static_cast<int>(buffer.size()), 0,
            reinterpret_cast<sockaddr*>(&source), &source_length);
        if (received >= 0)
        {
            char host[NI_MAXHOST]{};
            char service[NI_MAXSERV]{};
            if (getnameinfo(reinterpret_cast<sockaddr*>(&source),
                    source_length, host, sizeof(host), service,
                    sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV) != 0)
            {
                network::fail("operation_failed", "无法读取 UDP 来源地址");
            }
            const auto kept = std::min<std::size_t>(received,
                static_cast<std::size_t>(max_bytes));
            datagram result;
            result.data.assign(buffer.begin(), buffer.begin() + kept);
            result.host = host;
            result.port = std::stoll(service);
            result.truncated = static_cast<std::size_t>(received) > kept;
            return result;
        }
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            socket_error("接收 UDP 报文");
        }
    }
}

} // namespace tx_generated::socket
