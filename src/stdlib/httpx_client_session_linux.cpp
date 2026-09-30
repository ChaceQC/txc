#include "stdlib/httpx_client_session.hpp"
#include "stdlib/httpx_curl.hpp"

#include <algorithm>
#include <limits>
#include <unordered_map>

namespace tx_generated::httpx_session
{
namespace
{
struct registry
{
    std::mutex mutex;
    std::int64_t next_id = 1;
    std::unordered_map<std::int64_t, std::shared_ptr<httpx_curl::session>> sessions;
    std::unordered_map<std::int64_t, std::shared_ptr<httpx_curl::request>> requests;
};

registry& states()
{
    static registry value;
    return value;
}

template<class table_type>
auto lookup(table_type& table, std::int64_t id)
{
    const auto found = table.find(id);
    if (found == table.end())
    {
        network::fail("connection_closed", "HTTP 会话或请求已关闭");
    }
    return found->second;
}

std::shared_ptr<httpx_curl::request> request_at(std::int64_t id)
{
    auto& value = states();
    std::lock_guard lock(value.mutex);
    return lookup(value.requests, id);
}

std::int64_t open_configured(std::string_view proxy_url, std::int64_t maximum,
    bool decompress, bool allow_http2, std::shared_ptr<const httpx_client_tls::settings> tls)
{
    if (maximum < 1 || maximum > 64)
    {
        network::fail("invalid_argument", "HTTP 每主机连接数须为 1～64");
    }
    if (!proxy_url.empty() && proxy_url != "direct")
    {
        const auto proxy = network::parse_url(proxy_url, false);
        if (proxy.secure || proxy.target != L"/")
        {
            network::fail("invalid_url", "HTTP 代理只接受无路径的 http://主机:端口");
        }
    }
    auto session = std::make_shared<httpx_curl::session>(proxy_url, maximum, decompress, allow_http2);
    session->tls = std::move(tls);
    auto& value = states();
    std::lock_guard lock(value.mutex);
    const auto id = value.next_id++;
    value.sessions.emplace(id, std::move(session));
    return id;
}
}

std::int64_t open(std::string_view proxy_url, std::int64_t max_connections, bool decompress)
{
    return open_configured(proxy_url, max_connections, decompress, true, {});
}

std::int64_t open_secure(std::string_view proxy_url, std::int64_t max_connections,
    bool decompress, const bytes_vector& anchors, bool include_system,
    const byte_value& package, const secret::handle& password, bool allow_http2)
{
    std::shared_ptr<const httpx_client_tls::settings> tls;
    if (!anchors.data().values.empty() || !package->empty())
    {
        tls = httpx_client_tls::create(anchors, include_system, package, password);
    }
    return open_configured(proxy_url, max_connections, decompress, allow_http2, std::move(tls));
}

void close(std::int64_t id) noexcept
{
    auto& value = states();
    std::lock_guard lock(value.mutex);
    // 活动请求继续持有会话；移除入口仅禁止新请求，保持现有句柄生命周期。
    value.sessions.erase(id);
}

std::int64_t begin(std::int64_t id, std::string_view method, std::string_view url,
    const network::header_map& headers, std::int64_t body_length,
    std::int64_t max_response_bytes, std::int64_t timeout_ms, bool require_http2)
{
    if (body_length < 0 || body_length > std::numeric_limits<std::uint32_t>::max() ||
        max_response_bytes < 0 || timeout_ms < 1 || timeout_ms > std::numeric_limits<int>::max())
    {
        network::fail("invalid_argument", "HTTP 请求长度、接收上限或超时无效");
    }
    std::shared_ptr<httpx_curl::session> owner;
    {
        auto& value = states();
        std::lock_guard lock(value.mutex);
        owner = lookup(value.sessions, id);
    }
    std::lock_guard lock(owner->mutex);
    if (require_http2 && !owner->allow_http2)
    {
        network::fail("invalid_argument", "HTTP 会话未启用 HTTP/2");
    }
    if (require_http2 && !network::parse_url(url, false).secure)
    {
        network::fail("invalid_url", "会话 HTTP/2 请求只接受 HTTPS");
    }
    auto request = std::make_shared<httpx_curl::request>(owner);
    request->remaining_upload = body_length;
    request->response_limit = max_response_bytes;
    request->timeout_ms = timeout_ms;
    request->require_http2 = require_http2;
    httpx_curl::configure(*request, method, url, headers, body_length);
    auto& value = states();
    std::lock_guard registry_lock(value.mutex);
    const auto request_id = value.next_id++;
    value.requests.emplace(request_id, std::move(request));
    return request_id;
}

std::int64_t write(std::int64_t id, std::string_view data)
{
    auto value = request_at(id);
    std::lock_guard lock(value->owner->mutex);
    httpx_curl::check(*value);
    if (value->current != httpx_curl::phase::uploading)
    {
        network::fail("connection_closed", "HTTP 请求已结束上传");
    }
    if (data.size() > static_cast<std::uint64_t>(value->remaining_upload) ||
        data.size() > 16 * 1024 * 1024)
    {
        network::fail("size_limit", "HTTP 上传块超过声明长度或 16 MiB");
    }
    value->upload.assign(data);
    value->upload_offset = 0;
    value->remaining_upload -= static_cast<std::int64_t>(data.size());
    httpx_curl::resume(*value);
    httpx_curl::wait(*value, [&]()
    {
        return value->upload_offset == value->upload.size();
    });
    value->upload.clear();
    value->upload_offset = 0;
    return static_cast<std::int64_t>(data.size());
}

http_response_data finish(std::int64_t id)
{
    auto value = request_at(id);
    std::lock_guard lock(value->owner->mutex);
    if (value->current != httpx_curl::phase::uploading || value->remaining_upload != 0)
    {
        network::fail("invalid_argument", "HTTP 请求正文未完整写入或已提交");
    }
    httpx_curl::resume(*value);
    httpx_curl::wait(*value, [&]()
    {
        return value->header_ready;
    });
    long protocol = 0;
    curl_easy_getinfo(value->easy, CURLINFO_HTTP_VERSION, &protocol);
    value->response.protocol = protocol == CURL_HTTP_VERSION_2_0 ? "h2" : "http/1.1";
    if (value->require_http2 && protocol != CURL_HTTP_VERSION_2_0)
    {
        value->detach();
        network::fail("protocol_error", "HTTP 服务端未协商 HTTP/2");
    }
    value->current = httpx_curl::phase::reading;
    return value->response;
}

response_chunk read(std::int64_t id, std::int64_t max_bytes)
{
    if (max_bytes < 1 || max_bytes > 16 * 1024)
    {
        network::fail("invalid_argument", "HTTP 读取块须为 1～16384 字节");
    }
    auto value = request_at(id);
    std::lock_guard lock(value->owner->mutex);
    if (value->current == httpx_curl::phase::uploading)
    {
        network::fail("invalid_argument", "HTTP 请求尚未读取响应头");
    }
    httpx_curl::check(*value);
    httpx_curl::resume(*value);
    httpx_curl::wait(*value, [&]()
    {
        return !value->download.empty() || value->done;
    });
    if (value->download.empty())
    {
        value->current = httpx_curl::phase::eof;
        return {{}, true};
    }
    const auto amount = std::min<std::size_t>(value->download.size(), max_bytes);
    auto data = value->download.substr(0, amount);
    value->download.erase(0, amount);
    return {std::move(data), false};
}

void close_request(std::int64_t id) noexcept
{
    std::shared_ptr<httpx_curl::request> request;
    {
        auto& value = states();
        std::lock_guard lock(value.mutex);
        const auto found = value.requests.find(id);
        if (found == value.requests.end())
        {
            return;
        }
        request = std::move(found->second);
        value.requests.erase(found);
    }
    auto owner = request->owner;
    std::lock_guard lock(owner->mutex);
    request->detach();
    request.reset();
}
}
