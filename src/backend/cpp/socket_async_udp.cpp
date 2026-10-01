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

constexpr std::size_t maximum_datagram = 65507;

void check_endpoint(const std::shared_ptr<socket::state>& value)
{
    if (value->kind != socket::resource_kind::udp_socket)
    {
        network::fail("invalid_argument", "需要 UDP 句柄");
    }
}

struct target_address
{
    socket::address_list addresses{nullptr, freeaddrinfo};
};

} // namespace

std::shared_ptr<operation> make_send_to(std::shared_ptr<socket::state> endpoint,
    std::string ip, std::int64_t port, byte_value bytes,
    std::int64_t timeout_ms)
{
    auto value = std::make_shared<operation>();
    value->state = std::move(endpoint);
    value->timeout_ms = timeout_ms;
    value->write = true;
    auto target = std::make_shared<target_address>();
    value->start = [ip = std::move(ip), port, bytes,
        target](operation& pending) -> step_result
    {
        check_endpoint(pending.state);
        socket::check_port(port, false);
        socket::check_timeout(pending.timeout_ms);
        if (bytes->size() > maximum_datagram)
        {
            network::fail("size_limit", "UDP 报文超过 65507 字节");
        }
        target->addresses = socket::numeric_addresses(ip, port,
            SOCK_DGRAM, IPPROTO_UDP, false);
        pending.native = socket::native_socket(pending.state);
        return {};
    };
    value->ready = [bytes, target](operation& pending) -> step_result
    {
        auto* item = target->addresses.get();
        if (!item)
        {
            network::fail("invalid_argument", "UDP 目的地址无效");
        }
        const int sent = sendto(pending.native->get(),
            bytes->empty() ? "" :
                reinterpret_cast<const char*>(bytes->data()),
            static_cast<int>(bytes->size()), 0, item->ai_addr,
            static_cast<int>(item->ai_addrlen));
        if (sent >= 0)
        {
            if (static_cast<std::size_t>(sent) != bytes->size())
            {
                network::fail("short_write", "UDP 报文未完整发送");
            }
            return task_result(static_cast<std::int64_t>(sent));
        }
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            return {};
        }
        socket::socket_error("发送 UDP 报文");
    };
    return value;
}

std::shared_ptr<operation> make_receive_from(
    std::shared_ptr<socket::state> endpoint, std::int64_t max_bytes,
    std::int64_t timeout_ms, std::string result_name)
{
    auto value = std::make_shared<operation>();
    value->state = std::move(endpoint);
    value->timeout_ms = timeout_ms;
    auto buffer = std::make_shared<std::vector<std::uint8_t>>();
    value->start = [max_bytes, buffer](operation& pending) -> step_result
    {
        check_endpoint(pending.state);
        socket::check_timeout(pending.timeout_ms);
        if (max_bytes < 0 || max_bytes >
            static_cast<std::int64_t>(maximum_datagram))
        {
            network::fail("invalid_argument", "UDP 接收上限必须为 0～65507 字节");
        }
        pending.native = socket::native_socket(pending.state);
        buffer->resize(65536);
        return {};
    };
    value->ready = [max_bytes, result_name,
        buffer](operation& pending) -> step_result
    {
        sockaddr_storage source{};
        network::socket_length source_length = sizeof(source);
        const int received = recvfrom(pending.native->get(),
            reinterpret_cast<char*>(buffer->data()),
            static_cast<int>(buffer->size()), 0,
            reinterpret_cast<sockaddr*>(&source), &source_length);
        if (received < 0)
        {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                return {};
            }
            socket::socket_error("接收 UDP 报文");
        }
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
        socket::datagram packet;
        packet.data.assign(buffer->begin(), buffer->begin() + kept);
        packet.host = host;
        packet.port = std::stoll(service);
        packet.truncated = static_cast<std::size_t>(received) > kept;
        return task_result(std::any(socket_abi::datagram_value(
            result_name.c_str(), std::move(packet))));
    };
    return value;
}

} // namespace tx_generated::socket_async
