#include "stdlib/httpx_client_session.hpp"

#include "stdlib/encoding.hpp"
#include "stdlib/httpx_client_custom_tls.hpp"
#include "stdlib/httpx_client_tls.hpp"
#include "stdlib/httpx_winhttp.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace tx_generated::httpx_session
{
namespace
{

struct session_state
{
    network::http_handle handle;
    bool decompress = false;
    bool allow_http2 = false;
    std::int64_t max_connections = 0;
    std::string proxy_url;
    std::shared_ptr<const httpx_client_tls::settings> tls;
    std::shared_ptr<httpx_custom_tls::session_pool> custom_tls_pool;
};

enum class request_phase
{
    uploading,
    reading,
    eof
};

struct request_state
{
    std::mutex mutex;
    std::shared_ptr<session_state> session;
    network::http_handle connection;
    network::http_handle handle;
    request_phase phase = request_phase::uploading;
    std::uint64_t remaining_upload = 0;
    std::uint64_t received = 0;
    std::uint64_t response_limit = 0;
    std::uint64_t compression_limit = std::numeric_limits<std::uint64_t>::max();
    bool require_http2 = false;
    std::string hostname;
    std::shared_ptr<httpx_custom_tls::request> custom_tls;
};

struct registry
{
    std::mutex mutex;
    std::int64_t next_id = 1;
    std::unordered_map<std::int64_t, std::shared_ptr<session_state>> sessions;
    std::unordered_map<std::int64_t, std::shared_ptr<request_state>> requests;
};

registry& states()
{
    static registry result;
    return result;
}

std::shared_ptr<session_state> session_at(std::int64_t id)
{
    auto& value = states();
    std::lock_guard lock(value.mutex);
    const auto found = value.sessions.find(id);
    if (found == value.sessions.end())
    {
        network::fail("connection_closed", "HTTP 客户端会话已关闭");
    }
    return found->second;
}

std::shared_ptr<request_state> request_at(std::int64_t id)
{
    auto& value = states();
    std::lock_guard lock(value.mutex);
    const auto found = value.requests.find(id);
    if (found == value.requests.end())
    {
        network::fail("connection_closed", "HTTP 客户端请求已关闭");
    }
    return found->second;
}

std::wstring proxy_name(std::string_view url)
{
    const auto address = network::parse_url(url, false);
    if (address.secure || address.target != L"/")
    {
        network::fail("invalid_url", "HTTP 代理只接受无路径的 http://主机:端口");
    }
    std::wstring result = address.host;
    if (result.find(L':') != std::wstring::npos)
    {
        result = L"[" + result + L"]";
    }
    result += L":" + std::to_wstring(address.port);
    return result;
}

[[noreturn]] void request_failure(std::string_view action)
{
    const auto code = GetLastError();
    if (code == ERROR_WINHTTP_SECURE_FAILURE ||
        code == ERROR_WINHTTP_SECURE_FAILURE_PROXY ||
        code == ERROR_WINHTTP_SECURE_INVALID_CERT ||
        code == ERROR_WINHTTP_SECURE_INVALID_CA ||
        code == ERROR_WINHTTP_SECURE_CERT_DATE_INVALID ||
        code == ERROR_WINHTTP_SECURE_CERT_CN_INVALID ||
        code == ERROR_WINHTTP_SECURE_CERT_REVOKED ||
        code == ERROR_WINHTTP_SECURE_CERT_REV_FAILED ||
        code == ERROR_WINHTTP_SECURE_CERT_WRONG_USAGE ||
        code == ERROR_WINHTTP_SECURE_CHANNEL_ERROR ||
        code == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED ||
        code == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED_PROXY ||
        code == ERROR_WINHTTP_CLIENT_CERT_NO_PRIVATE_KEY ||
        code == ERROR_WINHTTP_CLIENT_CERT_NO_ACCESS_PRIVATE_KEY)
    {
        network::fail("security_error", std::string(action) + "失败，TLS 验证或客户端身份错误");
    }
    network::http_failure(action);
}

void configure_request(request_state& state, std::string_view method,
                       std::string_view url, const network::header_map& headers,
                       std::int64_t timeout_ms)
{
    network::validate_token(method, "HTTP 方法");
    const auto address = network::parse_url(url, false);
    state.hostname = detail::wide_to_utf8(address.host);
    if (state.require_http2 && !address.secure)
    {
        network::fail("invalid_url", "可复用会话的显式 HTTP/2 请求需要 HTTPS");
    }
    state.connection.reset(WinHttpConnect(state.session->handle.get(),
        address.host.c_str(), address.port, 0));
    if (!state.connection.get())
    {
        network::http_failure("创建 HTTP 连接");
    }
    const auto method_wide = detail::utf8_to_wide(method);
    state.handle.reset(WinHttpOpenRequest(state.connection.get(),
        method_wide.c_str(), address.target.c_str(), nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        address.secure ? WINHTTP_FLAG_SECURE : 0));
    if (!state.handle.get())
    {
        network::http_failure("创建 HTTP 请求");
    }
    if (state.session->tls)
    {
        if (!address.secure)
        {
            network::fail("invalid_url", "自定义 TLS 配置只用于 HTTPS");
        }
        httpx_client_tls::configure(state.handle.get(), *state.session->tls);
    }
    const auto timeout = static_cast<int>(timeout_ms);
    if (!WinHttpSetTimeouts(state.handle.get(), timeout, timeout,
                            timeout, timeout))
    {
        network::http_failure("设置 HTTP 超时");
    }
    // Cookie 由 requests 的域/路径 jar 管理，避免 WinHTTP 自带 jar 旁路其删除规则。
    DWORD disabled = WINHTTP_DISABLE_REDIRECTS | WINHTTP_DISABLE_COOKIES;
    if (!WinHttpSetOption(state.handle.get(), WINHTTP_OPTION_DISABLE_FEATURE,
                          &disabled, sizeof(disabled)))
    {
        network::http_failure("禁用 HTTP 自动重定向");
    }
    if (state.session->decompress)
    {
        DWORD flags = WINHTTP_DECOMPRESSION_FLAG_ALL;
        if (!WinHttpSetOption(state.handle.get(), WINHTTP_OPTION_DECOMPRESSION,
                              &flags, sizeof(flags)))
        {
            network::http_failure("启用 HTTP 解压");
        }
    }
    if (state.require_http2)
    {
        BOOL required = TRUE;
        if (!WinHttpSetOption(state.handle.get(),
                              WINHTTP_OPTION_HTTP_PROTOCOL_REQUIRED,
                              &required, sizeof(required)))
        {
            network::http_failure("要求 HTTP/2 协议");
        }
    }
    const auto header_text = httpx_winhttp::request_headers(headers);
    if (!WinHttpSendRequest(state.handle.get(), header_text.c_str(),
                            static_cast<DWORD>(header_text.size()),
                            WINHTTP_NO_REQUEST_DATA, 0,
                            static_cast<DWORD>(state.remaining_upload), 0))
    {
        request_failure("开始 HTTP 请求");
    }
}

} // namespace

std::int64_t open_configured(std::string_view proxy_url,
    std::int64_t max_connections, bool decompress, bool allow_http2)
{
    if (max_connections < 1 || max_connections > 64)
    {
        network::fail("invalid_argument", "HTTP 每主机连接数须为 1～64");
    }
    auto state = std::make_shared<session_state>();
    state->decompress = decompress;
    state->allow_http2 = allow_http2;
    state->max_connections = max_connections;
    state->proxy_url = std::string(proxy_url);
    std::wstring named_proxy;
    DWORD access = WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
    if (proxy_url == "direct")
    {
        access = WINHTTP_ACCESS_TYPE_NO_PROXY;
    }
    else if (!proxy_url.empty())
    {
        named_proxy = proxy_name(proxy_url);
        access = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    }
    state->handle.reset(WinHttpOpen(L"TX/1.0", access,
        named_proxy.empty() ? WINHTTP_NO_PROXY_NAME : named_proxy.c_str(),
        WINHTTP_NO_PROXY_BYPASS, 0));
    if (!state->handle.get())
    {
        network::http_failure("创建 HTTP 客户端会话");
    }
    DWORD limit = static_cast<DWORD>(max_connections);
    if (!WinHttpSetOption(state->handle.get(), WINHTTP_OPTION_MAX_CONNS_PER_SERVER,
                           &limit, sizeof(limit)))
    {
        network::http_failure("配置 HTTP 客户端连接池");
    }
    if (allow_http2)
    {
        DWORD protocols = WINHTTP_PROTOCOL_FLAG_HTTP2;
        if (!WinHttpSetOption(state->handle.get(),
                WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,
                &protocols, sizeof(protocols)))
        {
            network::http_failure("启用 HTTP/2");
        }
    }
    auto& value = states();
    std::lock_guard lock(value.mutex);
    const auto id = value.next_id++;
    value.sessions.emplace(id, std::move(state));
    return id;
}

std::int64_t open(std::string_view proxy_url, std::int64_t max_connections,
                  bool decompress)
{
    return open_configured(proxy_url, max_connections, decompress, true);
}

std::int64_t open_secure(std::string_view proxy_url,
    std::int64_t max_connections, bool decompress,
    const bytes_vector& anchors, bool include_system,
    const byte_value& package, const secret::handle& password,
    bool allow_http2)
{
    std::shared_ptr<const httpx_client_tls::settings> tls;
    if (!anchors.data().values.empty() || !package->empty())
    {
        tls = httpx_client_tls::create(anchors, include_system,
                                       package, password);
    }
    const auto id = open_configured(proxy_url, max_connections,
                                    decompress, allow_http2);
    auto state = session_at(id);
    if (tls && httpx_client_tls::has_custom_anchors(*tls))
    {
        state->custom_tls_pool =
            std::make_shared<httpx_custom_tls::session_pool>(tls,
                state->handle.get(), proxy_url, max_connections, decompress,
                allow_http2);
    }
    state->tls = std::move(tls);
    return id;
}

void close(std::int64_t session) noexcept
{
    auto& value = states();
    std::shared_ptr<session_state> closed;
    {
        std::lock_guard lock(value.mutex);
        const auto found = value.sessions.find(session);
        if (found != value.sessions.end())
        {
            closed = std::move(found->second);
            value.sessions.erase(found);
        }
    }
    if (closed && closed->custom_tls_pool)
    {
        closed->custom_tls_pool->close();
    }
}

std::int64_t begin(std::int64_t session, std::string_view method,
                   std::string_view url, const network::header_map& headers,
                   std::int64_t body_length, std::int64_t max_response_bytes,
                   std::int64_t timeout_ms, bool require_http2)
{
    if (body_length < 0 ||
        static_cast<std::uint64_t>(body_length) > std::numeric_limits<DWORD>::max() ||
        max_response_bytes < 0 || timeout_ms < 1 ||
        timeout_ms > std::numeric_limits<int>::max())
    {
        network::fail("invalid_argument", "HTTP 请求长度、接收上限或超时无效");
    }
    auto state = std::make_shared<request_state>();
    state->session = session_at(session);
    state->remaining_upload = static_cast<std::uint64_t>(body_length);
    state->response_limit = static_cast<std::uint64_t>(max_response_bytes);
    state->require_http2 = require_http2;
    if (state->session->custom_tls_pool)
    {
        state->custom_tls = std::make_shared<httpx_custom_tls::request>(
            state->session->custom_tls_pool, method, url,
            headers, body_length, max_response_bytes, timeout_ms,
            require_http2);
    }
    else
    {
        configure_request(*state, method, url, headers, timeout_ms);
    }
    auto& value = states();
    std::lock_guard lock(value.mutex);
    const auto id = value.next_id++;
    value.requests.emplace(id, std::move(state));
    return id;
}

std::int64_t write(std::int64_t request, std::string_view data)
{
    auto state = request_at(request);
    std::lock_guard lock(state->mutex);
    if (state->phase != request_phase::uploading)
    {
        network::fail("connection_closed", "HTTP 请求已结束上传");
    }
    if (data.size() > state->remaining_upload || data.size() > 16 * 1024 * 1024)
    {
        network::fail("size_limit", "HTTP 上传块超过声明长度或 16 MiB");
    }
    if (state->custom_tls)
    {
        const auto written = state->custom_tls->write(data);
        state->remaining_upload -= static_cast<std::uint64_t>(written);
        return written;
    }
    std::size_t offset = 0;
    while (offset < data.size())
    {
        DWORD written = 0;
        const auto count = static_cast<DWORD>(std::min<std::size_t>(
            16 * 1024, data.size() - offset));
        if (!WinHttpWriteData(state->handle.get(), data.data() + offset,
                              count, &written))
        {
            network::http_failure("写入 HTTP 请求正文");
        }
        if (written == 0)
        {
            network::fail("operation_failed", "HTTP 请求写入未取得进展");
        }
        offset += written;
    }
    state->remaining_upload -= offset;
    return static_cast<std::int64_t>(offset);
}

http_response_data finish(std::int64_t request)
{
    auto state = request_at(request);
    std::lock_guard lock(state->mutex);
    if (state->phase != request_phase::uploading || state->remaining_upload != 0)
    {
        network::fail("invalid_argument", "HTTP 请求正文未完整写入或已提交");
    }
    if (state->custom_tls)
    {
        auto result = state->custom_tls->finish();
        state->phase = request_phase::reading;
        return result;
    }
    if (!WinHttpReceiveResponse(state->handle.get(), nullptr))
    {
        request_failure("读取 HTTP 响应");
    }
    if (state->session->tls)
    {
        httpx_client_tls::verify(state->handle.get(), state->hostname,
                                 *state->session->tls);
    }
    if (state->require_http2)
    {
        httpx_winhttp::require_http2_protocol(state->handle.get());
    }
    auto result = httpx_winhttp::response_metadata(state->handle.get());
    if (!state->session->decompress &&
        result.headers.contains("content-length"))
    {
        (void)network::content_length(result.headers,
            static_cast<std::size_t>(state->response_limit));
    }
    state->phase = request_phase::reading;
    return result;
}

response_chunk read(std::int64_t request, std::int64_t max_bytes)
{
    if (max_bytes < 1 || max_bytes > 16 * 1024)
    {
        network::fail("invalid_argument", "HTTP 读取块须为 1～16384 字节");
    }
    auto state = request_at(request);
    std::lock_guard lock(state->mutex);
    if (state->phase == request_phase::uploading)
    {
        network::fail("invalid_argument", "HTTP 请求尚未读取响应头");
    }
    if (state->phase == request_phase::eof)
    {
        return {{}, true};
    }
    if (state->custom_tls)
    {
        auto result = state->custom_tls->read(max_bytes);
        if (result.eof)
        {
            state->phase = request_phase::eof;
        }
        return result;
    }
    std::array<char, 16 * 1024> buffer{};
    // 流式读取只消费当前可用数据，不能等到 max_bytes 填满才交付 SSE。
    DWORD available = 0;
    if (!WinHttpQueryDataAvailable(state->handle.get(), &available))
    {
        network::http_failure("等待 HTTP 响应正文");
    }
    DWORD received = 0;
    if (available > 0 && !WinHttpReadData(state->handle.get(), buffer.data(),
                          std::min(static_cast<DWORD>(max_bytes), available), &received))
    {
        network::http_failure("读取 HTTP 响应正文");
    }
    if (received == 0)
    {
        state->phase = request_phase::eof;
        state->handle.reset();
        state->connection.reset();
        return {{}, true};
    }
    if (state->session->decompress)
    {
        WINHTTP_REQUEST_STATS stats{};
        stats.cStats = WinHttpRequestStatMax;
        DWORD size = sizeof(stats);
        if (!WinHttpQueryOption(state->handle.get(), WINHTTP_OPTION_REQUEST_STATS,
                                &stats, &size) ||
            stats.cStats <= WinHttpResponseBodyCompressedSize)
        {
            state->handle.reset();
            state->connection.reset();
            state->phase = request_phase::eof;
            network::fail("operation_failed", "当前 WinHTTP 无法统计压缩传输字节数");
        }
        // WinHTTP 解压后会删除原始 Content-Length，故按实际编码字节计比率。
        constexpr std::uint64_t ratio = 100;
        constexpr std::uint64_t allowance = 1024;
        const auto encoded = stats.rgullStats[WinHttpResponseBodyCompressedSize];
        if (encoded > 0 &&
            encoded <= (std::numeric_limits<std::uint64_t>::max() -
                        allowance) / ratio)
        {
            state->compression_limit = encoded * ratio + allowance;
        }
    }
    if (received > state->response_limit - state->received ||
        received > state->compression_limit - state->received)
    {
        state->handle.reset();
        state->connection.reset();
        state->phase = request_phase::eof;
        network::fail("size_limit", "HTTP 响应正文超过大小或压缩比例上限");
    }
    state->received += received;
    return {{buffer.data(), received}, false};
}

void close_request(std::int64_t request) noexcept
{
    auto& value = states();
    std::lock_guard lock(value.mutex);
    value.requests.erase(request);
}

} // namespace tx_generated::httpx_session
