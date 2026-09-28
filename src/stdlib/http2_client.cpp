#include "stdlib/http2_client.hpp"

#include "stdlib/encoding.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <ws2tcpip.h>

namespace tx_generated::http2
{
namespace
{

struct winsock_session
{
    winsock_session()
    {
        WSADATA data{};
        const auto status = WSAStartup(MAKEWORD(2, 2), &data);
        if (status != 0)
        {
            network::fail("operation_failed", "初始化 HTTP/2 客户端网络失败");
        }
    }
    ~winsock_session()
    {
        WSACleanup();
    }
};

network::socket_handle connect_h2c(const network::parsed_url& address,
                                    std::int64_t timeout_ms)
{
    static winsock_session winsock;
    (void)winsock;
    const auto port = std::to_wstring(address.port);
    ADDRINFOW hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    ADDRINFOW* addresses = nullptr;
    if (GetAddrInfoW(address.host.c_str(), port.c_str(), &hints, &addresses) != 0)
    {
        network::fail("invalid_url", "无法解析 HTTP/2 主机");
    }
    std::unique_ptr<ADDRINFOW, decltype(&FreeAddrInfoW)> guard(addresses,
                                                               FreeAddrInfoW);
    bool timed_out = false;
    for (auto* item = addresses; item; item = item->ai_next)
    {
        network::socket_handle socket(::socket(item->ai_family,
            item->ai_socktype, item->ai_protocol));
        if (!socket.valid())
        {
            continue;
        }
        u_long nonblocking = 1;
        if (ioctlsocket(socket.get(), FIONBIO, &nonblocking) != 0)
        {
            continue;
        }
        const int connected = ::connect(socket.get(), item->ai_addr,
                                        static_cast<int>(item->ai_addrlen));
        if (connected != 0 && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            continue;
        }
        if (connected != 0)
        {
            fd_set writable;
            FD_ZERO(&writable);
            FD_SET(socket.get(), &writable);
            timeval limit{static_cast<long>(timeout_ms / 1000),
                          static_cast<long>((timeout_ms % 1000) * 1000)};
            const auto selected = select(0, nullptr, &writable, nullptr, &limit);
            if (selected <= 0)
            {
                timed_out = timed_out || selected == 0;
                continue;
            }
            int error = 0;
            int length = sizeof(error);
            if (getsockopt(socket.get(), SOL_SOCKET, SO_ERROR,
                           reinterpret_cast<char*>(&error), &length) != 0 ||
                error != 0)
            {
                continue;
            }
        }
        nonblocking = 0;
        if (ioctlsocket(socket.get(), FIONBIO, &nonblocking) != 0)
        {
            continue;
        }
        const DWORD timeout = static_cast<DWORD>(timeout_ms);
        setsockopt(socket.get(), SOL_SOCKET, SO_SNDTIMEO,
                   reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        return socket;
    }
    network::fail(timed_out ? "timeout" : "operation_failed",
                  "连接 HTTP/2 主机失败或超时");
}

} // namespace

client_session::client_session(network::socket_handle socket,
                               std::int64_t timeout_ms)
    : io_(std::move(socket), {}, timeout_ms)
{
    initialize();
}

client_session::client_session(std::shared_ptr<tls::secure_connection> secure,
                               std::int64_t timeout_ms)
    : io_(std::move(secure), timeout_ms)
{
    initialize();
}

void client_session::initialize()
{
    nghttp2_session_callbacks* callbacks = nullptr;
    if (nghttp2_session_callbacks_new(&callbacks) != 0)
    {
        network::fail("operation_failed", "创建 HTTP/2 客户端回调失败");
    }
    nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks,
                                                                on_data_chunk);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks,
                                                           on_frame_recv);
    const auto status = nghttp2_session_client_new(&session_, callbacks, this);
    nghttp2_session_callbacks_del(callbacks);
    if (status != 0)
    {
        network::fail("operation_failed", "创建 HTTP/2 客户端会话失败");
    }
    if (nghttp2_submit_settings(session_, NGHTTP2_FLAG_NONE, nullptr, 0) != 0)
    {
        nghttp2_session_del(session_);
        session_ = nullptr;
        network::fail("operation_failed", "配置 HTTP/2 客户端失败");
    }
}

client_session::~client_session() noexcept
{
    if (session_)
    {
        nghttp2_session_del(session_);
    }
}

std::vector<std::pair<std::string, std::string>>
client_session::request_headers(std::string_view method,
                                std::string_view scheme,
                                std::string_view target,
                                std::string_view authority,
                                const network::header_map& headers,
                                std::int64_t body_length)
{
    if (scheme != "http" && scheme != "https")
    {
        network::fail("invalid_argument", "HTTP/2 请求协议方案无效");
    }
    std::vector<std::pair<std::string, std::string>> result;
    result.emplace_back(":method", method);
    result.emplace_back(":scheme", scheme);
    result.emplace_back(":authority", authority);
    result.emplace_back(":path", target);
    std::size_t total = 0;
    for (const auto& [name, value] : result)
    {
        total += name.size() + value.size();
    }
    for (const auto& [name, value] : headers)
    {
        network::validate_header(name, value);
        const auto key = network::lower_ascii(name);
        if (key == "host" || key == "content-length" ||
            key == "transfer-encoding" || key == "connection" ||
            key == "proxy-connection" || key == "keep-alive" ||
            key == "upgrade")
        {
            network::fail("invalid_header", "HTTP/2 客户端不能覆盖连接字段");
        }
        result.emplace_back(key, value);
        total += key.size() + value.size();
    }
    const auto length_text = std::to_string(body_length);
    result.emplace_back("content-length", length_text);
    total += 14 + length_text.size();
    if (total > network::max_head_bytes)
    {
        network::fail("size_limit", "HTTP/2 请求头超过 64 KiB");
    }
    return result;
}

void client_session::flush_output()
{
    while (true)
    {
        const std::uint8_t* bytes = nullptr;
        const auto length = nghttp2_session_mem_send2(session_, &bytes);
        check_error();
        if (length < 0)
        {
            network::fail("protocol_error", "发送 HTTP/2 请求帧失败");
        }
        if (length == 0)
        {
            return;
        }
        io_.write_all({reinterpret_cast<const char*>(bytes),
                       static_cast<std::size_t>(length)});
    }
}

void client_session::read_input()
{
    const auto bytes = io_.read_some(16 * 1024);
    if (bytes.empty())
    {
        network::fail("connection_closed", "HTTP/2 响应提前结束");
    }
    std::size_t offset = 0;
    while (offset < bytes.size())
    {
        const auto consumed = nghttp2_session_mem_recv2(session_,
            reinterpret_cast<const std::uint8_t*>(bytes.data() + offset),
            bytes.size() - offset);
        check_error();
        if (consumed <= 0)
        {
            network::fail("protocol_error", "接收 HTTP/2 响应帧失败");
        }
        offset += static_cast<std::size_t>(consumed);
    }
}

void client_session::submit_request(std::string_view method,
    std::string_view scheme, std::string_view target,
    std::string_view authority, const network::header_map& headers,
    std::int64_t body_length)
{
    auto header_values = request_headers(method, scheme, target, authority,
                                         headers, body_length);
    std::vector<nghttp2_nv> fields;
    fields.reserve(header_values.size());
    for (auto& [name, value] : header_values)
    {
        fields.push_back({reinterpret_cast<std::uint8_t*>(name.data()),
                          reinterpret_cast<std::uint8_t*>(value.data()),
                          name.size(), value.size(), NGHTTP2_NV_FLAG_NONE});
    }
    nghttp2_data_provider2 provider{};
    provider.source.ptr = this;
    provider.read_callback = read_request_data;
    stream_id_ = nghttp2_submit_request2(session_, nullptr, fields.data(),
        fields.size(), body_length == 0 ? nullptr : &provider, nullptr);
    if (stream_id_ < 0)
    {
        network::fail("operation_failed", "提交 HTTP/2 请求失败");
    }
}

http_response_data client_session::run(std::string_view method,
    std::string_view scheme,
    std::string_view target, std::string_view authority,
    const network::header_map& headers, std::string_view body,
    const binary_stream& source, std::int64_t body_length,
    const binary_stream& destination, std::int64_t max_response_bytes,
    bool binary)
{
    source_ = source;
    body_ = body;
    request_remaining_ = static_cast<std::uint64_t>(body_length);
    destination_ = destination;
    max_response_bytes_ = max_response_bytes;
    binary_ = binary;
    submit_request(method, scheme, target, authority, headers, body_length);
    while (!complete_)
    {
        flush_output();
        if (!complete_)
        {
            read_input();
        }
    }
    if (response_.status == 0)
    {
        network::fail("protocol_error", "HTTP/2 响应缺少状态码");
    }
    if (response_.headers.contains("content-length") &&
        network::content_length(response_.headers,
            static_cast<std::size_t>(max_response_bytes)) !=
        static_cast<std::size_t>(response_.body_length))
    {
        network::fail("protocol_error", "HTTP/2 响应长度与正文不符");
    }
    if (!binary && !destination)
    {
        network::validate_utf8(response_.body);
    }
    return std::move(response_);
}

http_response_data client_send(std::string_view method, std::string_view url,
    const network::header_map& headers, std::string_view body,
    const binary_stream& source, std::int64_t body_length,
    const binary_stream& destination, std::int64_t max_response_bytes,
    std::int64_t timeout_ms, bool binary)
{
    if (timeout_ms <= 0 || timeout_ms > std::numeric_limits<int>::max() ||
        body_length < 0 || max_response_bytes < 0)
    {
        network::fail("invalid_argument", "HTTP/2 客户端长度或超时无效");
    }
    network::validate_token(method, "HTTP 方法");
    const auto address = network::parse_url(url, false);
    if (address.secure)
    {
        network::fail("invalid_url", "h2c 客户端只接受 http:// URL");
    }
    if (source && body_length > 0)
    {
        source->file.require_open();
    }
    if (destination)
    {
        destination->file.require_open();
    }
    if (!source && static_cast<std::uint64_t>(body_length) != body.size())
    {
        network::fail("invalid_argument", "HTTP/2 请求正文长度不匹配");
    }
    if (!binary && !source)
    {
        network::validate_utf8(body);
    }
    auto authority = detail::wide_to_utf8(address.host);
    if (authority.find(':') != std::string::npos)
    {
        authority = "[" + authority + "]";
    }
    if (address.port != 80)
    {
        authority += ":" + std::to_string(address.port);
    }
    client_session session(connect_h2c(address, timeout_ms), timeout_ms);
    return session.run(method, "http", detail::wide_to_utf8(address.target), authority,
                       headers, body, source, body_length, destination,
                       max_response_bytes, binary);
}

} // namespace tx_generated::http2
