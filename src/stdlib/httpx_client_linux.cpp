#include "stdlib/httpx.hpp"
#include "stdlib/httpx_client_session.hpp"

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
    static response_registry value;
    return value;
}

struct client_request
{
    std::int64_t session = httpx_session::open({}, 1, false);
    std::int64_t request = 0;
    ~client_request()
    {
        httpx_session::close_request(request);
        httpx_session::close(session);
    }
};
}

std::int64_t httpx_client_send(std::string_view method, std::string_view url,
    const network::header_map& headers, std::string_view body,
    std::int64_t timeout_ms, bool binary, bool http2)
{
    if (!binary)
    {
        network::validate_utf8(body);
    }
    if (body.size() > network::max_body_bytes)
    {
        network::fail("size_limit", "HTTP 请求正文超过 8 MiB");
    }
    client_request current;
    current.request = httpx_session::begin(current.session, method, url, headers,
        body.size(), network::max_body_bytes, timeout_ms, http2);
    httpx_session::write(current.request, body);
    auto result = httpx_session::finish(current.request);
    while (true)
    {
        auto chunk = httpx_session::read(current.request, 16 * 1024);
        if (chunk.eof)
        {
            break;
        }
        result.body += chunk.data;
    }
    if (!binary)
    {
        network::validate_utf8(result.body);
    }
    result.body_length = static_cast<std::int64_t>(result.body.size());
    auto& registry = responses();
    std::lock_guard lock(registry.mutex);
    const auto id = registry.next_id++;
    registry.values.emplace(id, std::move(result));
    return id;
}

http_response_data httpx_client_stream(std::string_view method,
    std::string_view url, const network::header_map& headers,
    const binary_stream& source, std::int64_t source_length,
    const binary_stream& destination, std::int64_t max_response_bytes,
    std::int64_t timeout_ms, bool http2)
{
    if (!destination || (source_length > 0 && !source) || source_length < 0 ||
        source_length > std::numeric_limits<std::uint32_t>::max() || max_response_bytes < 0)
    {
        network::fail("invalid_argument", "HTTP 流式长度、文件流或接收上限无效");
    }
    destination->file.require_open();
    if (source_length > 0)
    {
        source->file.require_open();
    }
    client_request current;
    current.request = httpx_session::begin(current.session, method, url, headers,
        source_length, max_response_bytes, timeout_ms, http2);
    std::int64_t remaining = source_length;
    char buffer[16 * 1024];
    while (remaining > 0)
    {
        const auto count = source->file.read_into(buffer,
            static_cast<std::size_t>(std::min<std::int64_t>(remaining, sizeof(buffer))));
        if (count == 0)
        {
            network::fail("operation_failed", "HTTP 请求源流提前结束");
        }
        httpx_session::write(current.request, {buffer, count});
        remaining -= static_cast<std::int64_t>(count);
    }
    auto result = httpx_session::finish(current.request);
    while (true)
    {
        const auto chunk = httpx_session::read(current.request, 16 * 1024);
        if (chunk.eof)
        {
            return result;
        }
        destination->file.write(chunk.data);
        result.body_length += static_cast<std::int64_t>(chunk.data.size());
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
}
