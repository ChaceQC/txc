#include "stdlib/httpx.hpp"
#include "stdlib/http2_client.hpp"
#include "stdlib/httpx_winhttp.hpp"

#include "stdlib/encoding.hpp"

#include <algorithm>
#include <limits>
#include <mutex>
#include <unordered_map>

namespace tx_generated
{
namespace
{

struct response_registry
{
    std::mutex mutex;
    std::int64_t next_id = 1;
    std::unordered_map<std::int64_t, http_response_data> values;
};

response_registry& responses()
{
    static response_registry result;
    return result;
}

struct client_handles
{
    network::http_handle session;
    network::http_handle connection;
    network::http_handle request;
};

client_handles open_request(std::string_view method, std::string_view url,
                            const network::header_map& headers,
                            std::int64_t timeout_ms, bool require_http2)
{
    network::validate_token(method, "HTTP 方法");
    if (timeout_ms <= 0 || timeout_ms > std::numeric_limits<int>::max())
    {
        network::fail("invalid_argument", "HTTP 客户端超时必须为正数");
    }
    const auto address = network::parse_url(url, false);
    const auto method_wide = detail::utf8_to_wide(method);
    client_handles result;
    result.session.reset(WinHttpOpen(L"TX/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0));
    if (!result.session.get())
    {
        network::http_failure("创建 HTTP 会话");
    }
    const auto timeout = static_cast<int>(timeout_ms);
    if (!WinHttpSetTimeouts(result.session.get(), timeout, timeout, timeout, timeout))
    {
        network::http_failure("设置 HTTP 超时");
    }
    if (require_http2)
    {
        DWORD protocols = WINHTTP_PROTOCOL_FLAG_HTTP2;
        if (!WinHttpSetOption(result.session.get(),
                              WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,
                              &protocols, sizeof(protocols)))
        {
            network::http_failure("启用 HTTP/2");
        }
    }
    result.connection.reset(WinHttpConnect(result.session.get(),
        address.host.c_str(), address.port, 0));
    if (!result.connection.get())
    {
        network::http_failure("连接 HTTP 主机");
    }
    result.request.reset(WinHttpOpenRequest(result.connection.get(),
        method_wide.c_str(), address.target.c_str(), nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        address.secure ? WINHTTP_FLAG_SECURE : 0));
    if (!result.request.get())
    {
        network::http_failure("创建 HTTP 请求");
    }
    DWORD disabled = WINHTTP_DISABLE_REDIRECTS;
    if (!WinHttpSetOption(result.request.get(), WINHTTP_OPTION_DISABLE_FEATURE,
                          &disabled, sizeof(disabled)))
    {
        network::http_failure("禁用 HTTP 自动重定向");
    }
    if (require_http2)
    {
        BOOL required = TRUE;
        if (!WinHttpSetOption(result.request.get(),
                              WINHTTP_OPTION_HTTP_PROTOCOL_REQUIRED,
                              &required, sizeof(required)))
        {
            network::http_failure("要求 HTTP/2 协议");
        }
    }
    (void)headers;
    return result;
}

std::int64_t register_response(http_response_data value)
{
    auto& registry = responses();
    std::lock_guard lock(registry.mutex);
    const auto id = registry.next_id++;
    registry.values.emplace(id, std::move(value));
    return id;
}

http_response_data receive_response(HINTERNET request, bool binary)
{
    auto result = httpx_winhttp::response_metadata(request);
    if (result.headers.contains("content-length"))
    {
        (void)network::content_length(result.headers);
    }
    char buffer[8192];
    while (true)
    {
        DWORD received = 0;
        if (!WinHttpReadData(request, buffer, sizeof(buffer), &received))
        {
            network::http_failure("读取 HTTP 正文");
        }
        if (received == 0)
        {
            break;
        }
        if (result.body.size() + received > network::max_body_bytes)
        {
            network::fail("size_limit", "HTTP 正文超过 8 MiB");
        }
        result.body.append(buffer, received);
    }
    if (!binary)
    {
        network::validate_utf8(result.body);
    }
    result.body_length = static_cast<std::int64_t>(result.body.size());
    return result;
}

} // namespace

std::int64_t httpx_client_send(std::string_view method, std::string_view url,
                               const network::header_map& headers,
                               std::string_view body, std::int64_t timeout_ms,
                               bool binary, bool http2)
{
    if (!binary)
    {
        network::validate_utf8(body);
    }
    if (body.size() > network::max_body_bytes)
    {
        network::fail("size_limit", "HTTP 请求正文超过 8 MiB");
    }
    if (http2 && !network::parse_url(url, false).secure)
    {
        auto response = http2::client_send(method, url, headers, body,
            {}, static_cast<std::int64_t>(body.size()), {},
            network::max_body_bytes, timeout_ms, binary);
        response.protocol = "h2";
        return register_response(std::move(response));
    }
    auto handles = open_request(method, url, headers, timeout_ms, http2);
    const auto header_text = httpx_winhttp::request_headers(headers);
    if (!WinHttpSendRequest(handles.request.get(), header_text.c_str(),
                            static_cast<DWORD>(header_text.size()),
                            body.empty() ? WINHTTP_NO_REQUEST_DATA
                                         : const_cast<char*>(body.data()),
                            static_cast<DWORD>(body.size()),
                            static_cast<DWORD>(body.size()), 0) ||
        !WinHttpReceiveResponse(handles.request.get(), nullptr))
    {
        network::http_failure("发送 HTTP 请求");
    }
    if (http2)
    {
        httpx_winhttp::require_http2_protocol(handles.request.get());
    }
    return register_response(receive_response(handles.request.get(), binary));
}

http_response_data httpx_client_stream(std::string_view method,
                                       std::string_view url,
                                       const network::header_map& headers,
                                       const binary_stream& source,
                                       std::int64_t source_length,
                                       const binary_stream& destination,
                                       std::int64_t max_response_bytes,
                                       std::int64_t timeout_ms, bool http2)
{
    if (source_length < 0 ||
        static_cast<std::uint64_t>(source_length) > std::numeric_limits<DWORD>::max() ||
        max_response_bytes < 0)
    {
        network::fail("invalid_argument", "HTTP 流式长度或接收上限无效");
    }
    if (!destination || (source_length > 0 && !source))
    {
        network::fail("invalid_argument", "HTTP 文件流未打开");
    }
    destination->file.require_open();
    if (source_length > 0)
    {
        source->file.require_open();
    }
    if (http2 && !network::parse_url(url, false).secure)
    {
        return http2::client_send(method, url, headers, {}, source,
            source_length, destination, max_response_bytes, timeout_ms, true);
    }
    auto handles = open_request(method, url, headers, timeout_ms, http2);
    const auto header_text = httpx_winhttp::request_headers(headers);
    if (!WinHttpSendRequest(handles.request.get(), header_text.c_str(),
                            static_cast<DWORD>(header_text.size()),
                            WINHTTP_NO_REQUEST_DATA, 0,
                            static_cast<DWORD>(source_length), 0))
    {
        network::http_failure("开始 HTTP 流式请求");
    }
    constexpr std::size_t block_size = 16 * 1024;
    char upload_buffer[block_size];
    auto remaining = static_cast<std::uint64_t>(source_length);
    while (remaining > 0)
    {
        const auto count = source->file.read_into(upload_buffer,
            static_cast<std::size_t>(std::min<std::uint64_t>(remaining,
                                                            block_size)));
        if (count == 0)
        {
            network::fail("operation_failed", "HTTP 请求源流提前结束");
        }
        std::size_t offset = 0;
        while (offset < count)
        {
            DWORD written = 0;
            if (!WinHttpWriteData(handles.request.get(), upload_buffer + offset,
                                  static_cast<DWORD>(count - offset), &written))
            {
                network::http_failure("写入 HTTP 请求正文");
            }
            if (written == 0)
            {
                network::fail("operation_failed", "HTTP 请求写入未取得进展");
            }
            offset += written;
        }
        remaining -= count;
    }
    if (!WinHttpReceiveResponse(handles.request.get(), nullptr))
    {
        network::http_failure("读取 HTTP 响应");
    }
    if (http2)
    {
        httpx_winhttp::require_http2_protocol(handles.request.get());
    }
    auto result = httpx_winhttp::response_metadata(handles.request.get());
    if (result.headers.contains("content-length"))
    {
        (void)network::content_length(result.headers,
            static_cast<std::size_t>(max_response_bytes));
    }
    char buffer[block_size];
    while (true)
    {
        DWORD received = 0;
        if (!WinHttpReadData(handles.request.get(), buffer, sizeof(buffer), &received))
        {
            network::http_failure("读取 HTTP 响应正文");
        }
        if (received == 0)
        {
            return result;
        }
        if (received > max_response_bytes - result.body_length)
        {
            network::fail("size_limit", "HTTP 响应正文超过接收上限");
        }
        destination->file.write({buffer, received});
        result.body_length += received;
    }
}

http_response_data httpx_response(std::int64_t id)
{
    auto& registry = responses();
    std::lock_guard lock(registry.mutex);
    const auto found = registry.values.find(id);
    if (found == registry.values.end())
    {
        network::fail("connection_closed", "HTTP 响应结果已释放");
    }
    return found->second;
}

void httpx_release_response(std::int64_t id) noexcept
{
    auto& registry = responses();
    std::lock_guard lock(registry.mutex);
    registry.values.erase(id);
}

} // namespace tx_generated
