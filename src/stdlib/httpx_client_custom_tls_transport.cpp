#include "stdlib/httpx_client_custom_tls_transport.hpp"

#include "stdlib/encoding.hpp"
#include "stdlib/error.hpp"
#include "stdlib/socket.hpp"
#include "stdlib/tls_stream.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <climits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <ws2tcpip.h>

namespace tx_generated::httpx_custom_tls
{
namespace
{

using clock = std::chrono::steady_clock;

void wait_socket(SOCKET socket, bool write,
                 const clock::time_point& deadline)
{
    while (true)
    {
        const auto now = clock::now();
        if (now >= deadline)
        {
            network::fail("timeout", "HTTP TLS 连接或传输超时");
        }
        const auto remaining = std::chrono::duration_cast<
            std::chrono::milliseconds>(deadline - now).count();
        fd_set ready;
        FD_ZERO(&ready);
        FD_SET(socket, &ready);
        timeval limit{static_cast<long>(remaining / 1000),
                      static_cast<long>((remaining % 1000) * 1000)};
        const int selected = select(0, write ? nullptr : &ready,
            write ? &ready : nullptr, nullptr, &limit);
        if (selected > 0)
        {
            return;
        }
        if (selected < 0)
        {
            network::socket_failure("等待 HTTP TLS 网络就绪");
        }
    }
}

std::shared_ptr<network::socket_handle> connect_host(
    std::wstring_view host, INTERNET_PORT port, std::int64_t timeout_ms)
{
    network::initialize_winsock();
    const auto port_text = std::to_wstring(port);
    ADDRINFOW hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    ADDRINFOW* addresses = nullptr;
    if (GetAddrInfoW(std::wstring(host).c_str(), port_text.c_str(),
            &hints, &addresses) != 0)
    {
        network::fail("invalid_url", "无法解析 HTTP TLS 主机");
    }
    std::unique_ptr<ADDRINFOW, decltype(&FreeAddrInfoW)> guard(
        addresses, FreeAddrInfoW);
    const auto deadline = clock::now() + std::chrono::milliseconds(timeout_ms);
    for (auto* address = addresses; address; address = address->ai_next)
    {
        network::socket_handle candidate(WSASocketW(address->ai_family,
            address->ai_socktype, address->ai_protocol, nullptr, 0,
            WSA_FLAG_OVERLAPPED));
        if (!candidate.valid())
        {
            continue;
        }
        u_long nonblocking = 1;
        if (ioctlsocket(candidate.get(), FIONBIO, &nonblocking) != 0)
        {
            continue;
        }
        if (connect(candidate.get(), address->ai_addr,
                static_cast<int>(address->ai_addrlen)) == 0)
        {
            return std::make_shared<network::socket_handle>(
                std::move(candidate));
        }
        const int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS)
        {
            continue;
        }
        try
        {
            wait_socket(candidate.get(), true, deadline);
        }
        catch (const runtime_failure& failure)
        {
            if (failure.error().code == "timeout")
            {
                throw;
            }
            continue;
        }
        int socket_error = 0;
        int size = sizeof(socket_error);
        if (getsockopt(candidate.get(), SOL_SOCKET, SO_ERROR,
                reinterpret_cast<char*>(&socket_error), &size) != 0 ||
            socket_error != 0)
        {
            continue;
        }
        return std::make_shared<network::socket_handle>(std::move(candidate));
    }
    network::fail("operation_failed", "连接 HTTP TLS 主机失败");
}

void send_raw(SOCKET socket, std::string_view data,
              const clock::time_point& deadline)
{
    while (!data.empty())
    {
        const int sent = send(socket, data.data(), static_cast<int>(
            std::min<std::size_t>(data.size(), 16 * 1024)), 0);
        if (sent > 0)
        {
            data.remove_prefix(static_cast<std::size_t>(sent));
            continue;
        }
        if (sent == 0)
        {
            network::fail("connection_closed", "HTTP 代理提前关闭连接");
        }
        const int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK)
        {
            network::socket_failure("发送 HTTP 代理请求");
        }
        wait_socket(socket, true, deadline);
    }
}

std::string read_raw_head(SOCKET socket,
                          const clock::time_point& deadline)
{
    std::string result;
    while (result.find("\r\n\r\n") == std::string::npos)
    {
        if (result.size() >= network::max_head_bytes)
        {
            network::fail("size_limit", "HTTP 代理响应头超过 64 KiB");
        }
        char buffer[4096];
        const int received = recv(socket, buffer, sizeof(buffer), 0);
        if (received > 0)
        {
            result.append(buffer, static_cast<std::size_t>(received));
            continue;
        }
        if (received == 0)
        {
            network::fail("connection_closed", "HTTP 代理提前关闭连接");
        }
        const int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK)
        {
            network::socket_failure("读取 HTTP 代理响应");
        }
        wait_socket(socket, false, deadline);
    }
    const auto end = result.find("\r\n\r\n") + 4;
    if (end != result.size())
    {
        network::fail("protocol_error", "HTTP 代理在 CONNECT 响应后发送了多余数据");
    }
    return result;
}

std::string authority(const network::parsed_url& address)
{
    auto host = detail::wide_to_utf8(address.host);
    if (host.find(':') != std::string::npos)
    {
        host = "[" + host + "]";
    }
    if (address.port != 443)
    {
        host += ":" + std::to_string(address.port);
    }
    return host;
}

std::string connect_authority(const network::parsed_url& address)
{
    auto host = detail::wide_to_utf8(address.host);
    if (host.find(':') != std::string::npos)
    {
        host = "[" + host + "]";
    }
    return host + ":" + std::to_string(address.port);
}


void connect_tunnel(SOCKET socket, const network::parsed_url& target,
                    std::int64_t timeout_ms)
{
    const auto host = connect_authority(target);
    const auto request = "CONNECT " + host + " HTTP/1.1\r\nHost: " + host +
        "\r\nProxy-Connection: Keep-Alive\r\n\r\n";
    const auto deadline = clock::now() + std::chrono::milliseconds(timeout_ms);
    send_raw(socket, request, deadline);
    auto head = network::parse_head(read_raw_head(socket, deadline));
    const auto first = std::string_view(head.first_line);
    const auto first_space = first.find(' ');
    if (!first.starts_with("HTTP/1.") || first_space == std::string_view::npos ||
        first_space + 4 > first.size())
    {
        network::fail("protocol_error", "HTTP 代理返回了无效 CONNECT 状态行");
    }
    int status = 0;
    const auto code = first.substr(first_space + 1, 3);
    const auto [end, error] = std::from_chars(code.data(),
        code.data() + code.size(), status);
    if (error != std::errc{} || end != code.data() + code.size() ||
        status < 200 || status >= 300)
    {
        network::fail("operation_failed", "HTTP 代理拒绝 TLS CONNECT 请求");
    }
}

void send_tls(const std::shared_ptr<tls::secure_connection>& connection,
              std::string_view data, std::int64_t timeout_ms)
{
    while (!data.empty())
    {
        const auto sent = connection->write(data, timeout_ms);
        if (sent <= 0)
        {
            network::fail("connection_closed", "发送 HTTP TLS 数据时连接已关闭");
        }
        data.remove_prefix(static_cast<std::size_t>(sent));
    }
}

std::string status_line(const network::parsed_head& head)
{
    const std::string_view line(head.first_line);
    if (!(line.starts_with("HTTP/1.0 ") || line.starts_with("HTTP/1.1 ")) ||
        line.size() < 12)
    {
        network::fail("protocol_error", "HTTP 响应状态行无效");
    }
    return head.first_line;
}

bool header_has_token(const network::header_map& headers,
                      std::string_view name, std::string_view token)
{
    const auto found = headers.find(std::string(name));
    if (found == headers.end())
    {
        return false;
    }
    std::string_view value(found->second);
    while (!value.empty())
    {
        const auto comma = value.find(',');
        auto item = value.substr(0, comma);
        while (!item.empty() && item.front() == ' ')
        {
            item.remove_prefix(1);
        }
        while (!item.empty() && item.back() == ' ')
        {
            item.remove_suffix(1);
        }
        if (network::lower_ascii(item) == token)
        {
            return true;
        }
        if (comma == std::string_view::npos)
        {
            break;
        }
        value.remove_prefix(comma + 1);
    }
    return false;
}

} // namespace

std::string request_authority(const network::parsed_url& address)
{
    return authority(address);
}

void send_tls_data(const std::shared_ptr<tls::secure_connection>& connection,
    std::string_view data, std::int64_t timeout_ms)
{
    send_tls(connection, data, timeout_ms);
}

std::string validate_status_line(const network::parsed_head& head)
{
    return status_line(head);
}

bool response_header_has_token(const network::header_map& headers,
    std::string_view name, std::string_view token)
{
    return header_has_token(headers, name, token);
}

std::shared_ptr<tls::secure_connection> open_verified_connection(
    const httpx_client_tls::settings& tls, std::string_view proxy_url,
    const network::parsed_url& target,
    const std::vector<std::string>& protocols, std::int64_t timeout_ms,
    bool allow_no_alpn)
{
    if (proxy_url != "direct")
    {
        const auto proxy = network::parse_url(proxy_url, false);
        if (proxy.secure || proxy.target != L"/")
        {
            network::fail("invalid_url",
                "HTTP TLS 代理只接受 http://主机:端口");
        }
        auto socket = connect_host(proxy.host, proxy.port, timeout_ms);
        connect_tunnel(socket->get(), target, timeout_ms);
        return httpx_client_tls::connect_custom(tls, std::move(socket),
            detail::wide_to_utf8(target.host), protocols, timeout_ms,
            allow_no_alpn);
    }
    auto socket = connect_host(target.host, target.port, timeout_ms);
    return httpx_client_tls::connect_custom(tls, std::move(socket),
        detail::wide_to_utf8(target.host), protocols, timeout_ms,
        allow_no_alpn);
}

} // namespace tx_generated::httpx_custom_tls
