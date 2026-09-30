#include "stdlib/dns.hpp"
#include "stdlib/network_common.hpp"
#include "stdlib/error.hpp"

#include <ares.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <memory>
#include <unordered_set>

namespace tx_generated::dns
{
namespace
{
using clock_type = std::chrono::steady_clock;

void check_token(const std::shared_ptr<cancellation_state>& token)
{
    if (!token)
    {
        return;
    }
    std::lock_guard lock(token->mutex);
    const bool expired = token->deadline && clock_type::now() >= *token->deadline;
    if (token->cancelled || expired)
    {
        throw runtime_failure({tx::error_kind::cancelled,
            token->cancelled ? "cancelled" : "deadline_exceeded",
            token->cancelled ? "DNS 解析已取消" : "DNS 解析截止时间已到"});
    }
}

struct resolver
{
    ares_channel channel = nullptr;
    resolver()
    {
        static const int initialized = ares_library_init(ARES_LIB_INIT_ALL);
        if (initialized != ARES_SUCCESS || ares_init(&channel) != ARES_SUCCESS)
        {
            network::fail("operation_failed", "初始化 DNS 解析器失败");
        }
    }
    ~resolver()
    {
        ares_destroy(channel);
    }
};

struct query
{
    bool completed = false;
    int status = ARES_SUCCESS;
    std::vector<address> addresses;
    std::exception_ptr failure;
};

void collect(void* context, int status, int, ares_addrinfo* addresses) noexcept
{
    auto& value = *static_cast<query*>(context);
    value.completed = true;
    value.status = status;
    std::unique_ptr<ares_addrinfo, decltype(&ares_freeaddrinfo)> guard(addresses, ares_freeaddrinfo);
    if (status != ARES_SUCCESS)
    {
        return;
    }
    try
    {
        std::unordered_set<std::string> seen;
        for (auto* item = addresses->nodes; item; item = item->ai_next)
        {
            if (item->ai_family != AF_INET && item->ai_family != AF_INET6)
            {
                continue;
            }
            const void* bytes = item->ai_family == AF_INET
                ? static_cast<const void*>(&reinterpret_cast<sockaddr_in*>(item->ai_addr)->sin_addr)
                : static_cast<const void*>(&reinterpret_cast<sockaddr_in6*>(item->ai_addr)->sin6_addr);
            char text[INET6_ADDRSTRLEN]{};
            if (!inet_ntop(item->ai_family, bytes, text, sizeof(text)))
            {
                network::fail("operation_failed", "格式化 DNS 地址失败");
            }
            if (seen.insert(text).second)
            {
                if (value.addresses.size() == 128)
                {
                    network::fail("size_limit", "DNS 地址记录超过 128 条");
                }
                value.addresses.push_back({text, item->ai_family == AF_INET ? "ipv4" : "ipv6",
                    std::max(0, item->ai_ttl)});
            }
        }
    }
    catch (...)
    {
        value.failure = std::current_exception();
    }
}

std::vector<address> literal(std::string_view host)
{
    std::array<unsigned char, 16> bytes{};
    const std::string name(host);
    for (int family : {AF_INET, AF_INET6})
    {
        if (inet_pton(family, name.c_str(), bytes.data()) == 1)
        {
            char text[INET6_ADDRSTRLEN]{};
            inet_ntop(family, bytes.data(), text, sizeof(text));
            return {{text, family == AF_INET ? "ipv4" : "ipv6", 0}};
        }
    }
    if (host.find(':') != std::string_view::npos)
    {
        network::fail("invalid_argument", "DNS 主机名不能包含端口或无效的 IPv6 地址");
    }
    return host == "localhost"
        ? std::vector<address>{{"127.0.0.1", "ipv4", 0}, {"::1", "ipv6", 0}}
        : std::vector<address>{};
}
}

std::vector<address> resolve(std::string_view host, std::int64_t timeout_ms,
    const std::shared_ptr<cancellation_state>& token)
{
    network::validate_utf8(host);
    if (host.empty() || host.size() > 253 ||
        host.find_first_of("/\\@?#\0 \t\r\n", 0, 10) != std::string_view::npos ||
        timeout_ms < 1 || timeout_ms > 60000)
    {
        network::fail("invalid_argument", "DNS 主机名或解析超时无效");
    }
    check_token(token);
    if (auto result = literal(host); !result.empty())
    {
        return result;
    }
    // 查询上下文先于 channel 构造，确保取消回调触发时上下文仍存活。
    query result;
    resolver source;
    const std::string name(host);
    ares_addrinfo_hints hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    // getaddrinfo 同时查询 A/AAAA，并遵循 c-ares 的系统 DNS 与 hosts 配置。
    ares_getaddrinfo(source.channel, name.c_str(), nullptr, &hints, collect, &result);
    const auto deadline = clock_type::now() + std::chrono::milliseconds(timeout_ms);
    while (!result.completed)
    {
        check_token(token);
        if (clock_type::now() >= deadline)
        {
            network::fail("timeout", "DNS 解析超时");
        }
        ares_socket_t sockets[ARES_GETSOCK_MAXNUM]{};
        const auto mask = ares_getsock(source.channel, sockets, ARES_GETSOCK_MAXNUM);
        std::vector<pollfd> descriptors;
        for (int index = 0; index < ARES_GETSOCK_MAXNUM; ++index)
        {
            const short events = (ARES_GETSOCK_READABLE(mask, index) ? POLLIN : 0) |
                (ARES_GETSOCK_WRITABLE(mask, index) ? POLLOUT : 0);
            if (events != 0)
            {
                descriptors.push_back({sockets[index], events, 0});
            }
        }
        const int ready = poll(descriptors.data(), descriptors.size(), 10);
        if (ready < 0 && errno != EINTR)
        {
            network::socket_failure("等待 DNS 响应");
        }
        for (const auto& descriptor : descriptors)
        {
            ares_process_fd(source.channel,
                descriptor.revents & (POLLIN | POLLERR | POLLHUP) ? descriptor.fd : ARES_SOCKET_BAD,
                descriptor.revents & POLLOUT ? descriptor.fd : ARES_SOCKET_BAD);
        }
        ares_process_fd(source.channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
    }
    check_token(token);
    if (result.failure)
    {
        std::rethrow_exception(result.failure);
    }
    if (!result.addresses.empty())
    {
        return result.addresses;
    }
    if (result.status == ARES_ENOTFOUND)
    {
        network::fail("name_not_found", "DNS 主机名不存在");
    }
    if (result.status == ARES_SUCCESS || result.status == ARES_ENODATA)
    {
        network::fail("no_records", "DNS 主机名没有可用的 IPv4 或 IPv6 地址");
    }
    if (result.status == ARES_ETIMEOUT)
    {
        network::fail("timeout", "DNS 解析超时");
    }
    network::fail("operation_failed", "DNS 地址解析失败");
}
}
