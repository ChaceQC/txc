#include "stdlib/httpx_negotiation.hpp"

#include "stdlib/error.hpp"
#include "stdlib/http3_client.hpp"

namespace tx_generated
{
namespace
{

http_response_data send_existing(std::string_view method,
    std::string_view url, const network::header_map& headers,
    std::string_view body, std::int64_t timeout_ms,
    bool binary, bool require_http2)
{
    const auto id = httpx_client_send(method, url, headers, body,
        timeout_ms, binary, require_http2);
    try
    {
        auto result = httpx_response(id);
        httpx_release_response(id);
        return result;
    }
    catch (...)
    {
        httpx_release_response(id);
        throw;
    }
}

bool replay_safe(std::string_view method, std::string_view body)
{
    return body.empty() &&
        (method == "GET" || method == "HEAD" || method == "OPTIONS");
}

} // namespace

http_response_data httpx_send_negotiated(std::string_view method,
    std::string_view url, const network::header_map& headers,
    std::string_view body, std::int64_t timeout_ms,
    std::string_view policy, bool binary)
{
    if (timeout_ms < 1 || timeout_ms > 60000)
    {
        network::fail("invalid_argument", "HTTP 协议策略超时须为 1～60000 毫秒");
    }
    if (policy == "system")
    {
        return send_existing(method, url, headers, body, timeout_ms,
                             binary, false);
    }
    if (policy == "h2_only")
    {
        return send_existing(method, url, headers, body, timeout_ms,
                             binary, true);
    }
    if (policy != "h3_only" && policy != "h3_then_h2" &&
        policy != "h3_then_system")
    {
        network::fail("invalid_argument", "HTTP 协议策略无效");
    }
    if (!network::parse_url(url, false).secure)
    {
        network::fail("invalid_url", "HTTP/3 协议策略只接受 HTTPS URL");
    }
    if (policy != "h3_only" && !replay_safe(method, body))
    {
        network::fail("replay_unsafe", "可降级请求须为无正文的 GET、HEAD 或 OPTIONS");
    }
    try
    {
        return http3::client_send(method, url, headers, body,
                                  timeout_ms, binary);
    }
    catch (const runtime_failure& error)
    {
        if (policy == "h3_only" ||
            (error.error().code != "timeout" &&
             error.error().code != "unsupported_protocol"))
        {
            throw;
        }
    }
    return send_existing(method, url, headers, body, timeout_ms,
                         binary, policy == "h3_then_h2");
}

} // namespace tx_generated
