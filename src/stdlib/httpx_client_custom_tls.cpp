#include "stdlib/httpx_client_custom_tls.hpp"
#include "stdlib/httpx_client_custom_tls_transport.hpp"

#include "stdlib/encoding.hpp"
#include "stdlib/error.hpp"
#include "stdlib/http2_client.hpp"
#include "stdlib/socket.hpp"
#include "stdlib/tls_stream.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <ws2tcpip.h>

#include "stdlib/httpx_client_custom_tls_state.hpp"
#include "stdlib/httpx_client_custom_tls_transport.hpp"

#include "stdlib/encoding.hpp"
#include "stdlib/error.hpp"
#include "stdlib/http2_client.hpp"

#include <algorithm>
#include <charconv>
#include <limits>
#include <string>
#include <utility>

namespace tx_generated::httpx_custom_tls
{

request::request(std::shared_ptr<session_pool> pool,
    std::string_view method, std::string_view url,
    const network::header_map& headers, std::int64_t body_length,
    std::int64_t max_response_bytes, std::int64_t timeout_ms,
    bool require_http2)
    : value_(std::make_unique<state>())
{
    if (!pool || !pool->value_)
    {
        network::fail("connection_closed", "HTTP TLS 会话已关闭");
    }
    if (timeout_ms < 1 || timeout_ms > std::numeric_limits<int>::max() ||
        body_length < 0 ||
        max_response_bytes < 0)
    {
        network::fail("invalid_argument", "HTTP TLS 请求长度或超时无效");
    }
    network::validate_token(method, "HTTP 方法");
    const auto target = network::parse_url(url, false);
    if (!target.secure)
    {
        network::fail("invalid_url", "自定义 TLS 配置只用于 HTTPS");
    }
    auto& pool_data = *pool->value_;
    std::string effective_proxy(pool_data.proxy_url);
    if (effective_proxy.empty())
    {
        effective_proxy = resolve_system_proxy(pool_data.proxy_session,
            target.host, url, target.secure);
    }
    auto effective_headers = headers;
    if (pool_data.decompress)
    {
        const auto encoding = effective_headers.find("accept-encoding");
        if (encoding != effective_headers.end() &&
            network::lower_ascii(encoding->second) != "identity")
        {
            network::fail("unsupported_option",
                "自定义 CA TLS 请求只支持 identity Accept-Encoding");
        }
        if (encoding == effective_headers.end())
        {
            effective_headers.emplace("accept-encoding", "identity");
        }
    }
    value_->pool = std::move(pool);
    auto host_key = network::lower_ascii(detail::wide_to_utf8(target.host));
    value_->pool_key = host_key + ":" + std::to_string(target.port) + "|" +
                       effective_proxy;
    auto lease = pool_data.acquire(value_->pool_key, require_http2, timeout_ms);
    value_->lease_active = true;

    value_->method = std::string(method);
    value_->target = detail::wide_to_utf8(target.target);
    value_->target_authority = request_authority(target);
    value_->headers = std::move(effective_headers);
    value_->decompress = pool_data.decompress;
    value_->timeout_ms = timeout_ms;
    value_->upload_length = body_length;
    value_->upload_remaining = static_cast<std::uint64_t>(body_length);
    value_->response_limit = static_cast<std::uint64_t>(max_response_bytes);

    if (!lease.reserved_new)
    {
        value_->connection = std::move(lease.value.tls);
        value_->http2 = std::move(lease.value.http2);
        value_->uses_http2 = lease.value.uses_http2;
    }
    else
    {
        std::vector<std::string> protocols;
        if (require_http2)
        {
            protocols = {"h2"};
        }
        else if (pool_data.allow_http2)
        {
            protocols = {"h2", "http/1.1"};
        }
        const bool allow_no_alpn = pool_data.allow_http2 && !require_http2;
        try
        {
            value_->connection = open_verified_connection(
                *pool_data.tls_settings, effective_proxy, target, protocols,
                timeout_ms, allow_no_alpn);
            const auto negotiated = value_->connection->negotiated_alpn();
            if (require_http2 && negotiated != "h2")
            {
                network::fail("protocol_error", "HTTPS 服务端未协商 HTTP/2");
            }
            value_->uses_http2 = negotiated == "h2";
            if (value_->uses_http2)
            {
                value_->http2 = std::make_shared<http2::client_session>(
                    value_->connection, timeout_ms);
            }
        }
        catch (...)
        {
            pool_data.abandon(value_->pool_key);
            value_->lease_active = false;
            throw;
        }
    }

    if (value_->uses_http2)
    {
        value_->http2->start(value_->method, "https", value_->target,
            value_->target_authority, value_->headers, body_length,
            max_response_bytes);
        return;
    }

    std::string request_head(method);
    request_head += " " + detail::wide_to_utf8(target.target) + " HTTP/1.1\r\nHost: ";
    request_head += request_authority(target);
    request_head += "\r\nContent-Length: " + std::to_string(body_length);
    request_head += "\r\n";
    bool expect_continue = false;
    for (const auto& [name, value] : value_->headers)
    {
        network::validate_header(name, value);
        const auto lower = network::lower_ascii(name);
        if (lower == "host" || lower == "content-length" ||
            lower == "transfer-encoding" || lower == "connection" ||
            lower == "proxy-connection" || lower == "keep-alive" ||
            lower == "upgrade")
        {
            network::fail("invalid_header", "调用方不能覆盖 HTTP 连接管理字段");
        }
        if (lower == "expect" && network::lower_ascii(value) == "100-continue")
        {
            expect_continue = true;
        }
        request_head += name + ": " + value + "\r\n";
        if (request_head.size() > network::max_head_bytes)
        {
            network::fail("size_limit", "HTTP 请求头超过 64 KiB");
        }
    }
    request_head += "\r\n";
    if (expect_continue && body_length > 0)
    {
        network::fail("unsupported_option",
            "自定义 CA TLS 请求暂不支持 Expect: 100-continue");
    }
    send_tls_data(value_->connection, request_head, timeout_ms);
}

request::~request() noexcept
{
    close();
}

std::int64_t request::write(std::string_view data)
{
    if (value_->current != state::phase::uploading)
    {
        network::fail("connection_closed", "HTTP 请求已结束上传");
    }
    if (data.size() > value_->upload_remaining || data.size() > 16 * 1024 * 1024)
    {
        network::fail("size_limit", "HTTP 上传块超过声明长度或 16 MiB");
    }
    if (value_->uses_http2)
    {
        value_->http2->write_body(data);
    }
    else
    {
        send_tls_data(value_->connection, data, value_->timeout_ms);
    }
    value_->upload_remaining -= data.size();
    return static_cast<std::int64_t>(data.size());
}

http_response_data request::finish()
{
    if (value_->current != state::phase::uploading || value_->upload_remaining != 0)
    {
        network::fail("invalid_argument", "HTTP 请求正文未完整写入或已提交");
    }
    if (value_->uses_http2)
    {
        value_->http2->finish_upload();
        value_->response = value_->http2->response_headers();
        const auto content_encoding =
            value_->response.headers.find("content-encoding");
        if (value_->decompress &&
            content_encoding != value_->response.headers.end() &&
            network::lower_ascii(content_encoding->second) != "identity")
        {
            network::fail("protocol_error",
                "自定义 CA TLS 路径未能解码压缩响应");
        }
    }
    else
    {
        value_->read_response_head();
    }
    value_->current = state::phase::reading;
    return value_->response;
}

void request::close() noexcept
{
    if (value_)
    {
        value_->close_connection();
        value_->current = state::phase::eof;
    }
}

} // namespace tx_generated::httpx_custom_tls
