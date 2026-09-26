#include "stdlib/network_common.hpp"

#include <limits>
#include <memory>
#include <ws2tcpip.h>

namespace tx_generated::network
{
namespace
{

struct winsock_session
{
    winsock_session()
    {
        WSADATA info{};
        if (WSAStartup(MAKEWORD(2, 2), &info) != 0)
        {
            fail("operation_failed", "无法初始化 Winsock");
        }
    }
    ~winsock_session()
    {
        WSACleanup();
    }
};

void require_winsock()
{
    static winsock_session session;
    (void)session;
}

} // namespace

socket_handle listen_tcp(std::string_view host, std::int64_t port)
{
    require_winsock();
    validate_utf8(host);
    if (port < 1 || port > 65535 || host.find('\0') != std::string_view::npos)
    {
        fail("invalid_argument", "监听地址或端口无效");
    }
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    addrinfo* addresses = nullptr;
    const auto service = std::to_string(port);
    const std::string host_text(host);
    if (getaddrinfo(host.empty() ? nullptr : host_text.c_str(), service.c_str(),
                    &hints, &addresses) != 0)
    {
        fail("invalid_argument", "无法解析本机监听地址");
    }
    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> guard(addresses, freeaddrinfo);
    for (auto* item = addresses; item; item = item->ai_next)
    {
        socket_handle socket(::socket(item->ai_family, item->ai_socktype,
                                      item->ai_protocol));
        if (!socket.valid())
        {
            continue;
        }
        const BOOL exclusive = TRUE;
        setsockopt(socket.get(), SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                   reinterpret_cast<const char*>(&exclusive), sizeof(exclusive));
        if (item->ai_family == AF_INET6 && host.empty())
        {
            const DWORD dual_stack = 0;
            setsockopt(socket.get(), IPPROTO_IPV6, IPV6_V6ONLY,
                       reinterpret_cast<const char*>(&dual_stack), sizeof(dual_stack));
        }
        if (bind(socket.get(), item->ai_addr, static_cast<int>(item->ai_addrlen)) == 0 &&
            ::listen(socket.get(), SOMAXCONN) == 0)
        {
            return socket;
        }
    }
    socket_failure("绑定监听地址");
}

socket_handle accept_tcp(SOCKET listener, std::int64_t timeout_ms)
{
    if (timeout_ms < 0 || timeout_ms > std::numeric_limits<int>::max())
    {
        fail("invalid_argument", "等待超时参数无效");
    }
    if (timeout_ms != 0)
    {
        fd_set ready;
        FD_ZERO(&ready);
        FD_SET(listener, &ready);
        timeval limit{static_cast<long>(timeout_ms / 1000),
                      static_cast<long>((timeout_ms % 1000) * 1000)};
        const int selected = select(0, &ready, nullptr, nullptr, &limit);
        if (selected == 0)
        {
            fail("timeout", "等待连接超时");
        }
        if (selected < 0)
        {
            socket_failure("等待连接");
        }
    }
    socket_handle result(::accept(listener, nullptr, nullptr));
    if (!result.valid())
    {
        socket_failure("接受连接");
    }
    const DWORD send_timeout = 30000;
    setsockopt(result.get(), SOL_SOCKET, SO_SNDTIMEO,
               reinterpret_cast<const char*>(&send_timeout), sizeof(send_timeout));
    return result;
}

} // namespace tx_generated::network
