#include "stdlib/httpx.hpp"
#include "stdlib/http2_server.hpp"
#include "stdlib/error.hpp"

#include <algorithm>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace tx_generated
{
namespace
{

struct connection_state
{
    explicit connection_state(network::socket_handle socket)
        : stream(std::move(socket)) {}
    network::tcp_stream stream;
    http_request_data request;
};

struct server_registry
{
    std::mutex mutex;
    std::int64_t next_id = 1;
    std::unordered_map<std::int64_t, std::shared_ptr<network::socket_handle>> listeners;
    std::unordered_map<std::int64_t, std::shared_ptr<connection_state>> connections;
};

server_registry& servers()
{
    static server_registry result;
    return result;
}

std::shared_ptr<network::socket_handle> listener_at(std::int64_t id)
{
    auto& registry = servers();
    std::lock_guard lock(registry.mutex);
    const auto found = registry.listeners.find(id);
    if (found == registry.listeners.end())
    {
        network::fail("connection_closed", "HTTP 监听器已关闭");
    }
    return found->second;
}

std::shared_ptr<connection_state> connection_at(std::int64_t id)
{
    auto& registry = servers();
    std::lock_guard lock(registry.mutex);
    const auto found = registry.connections.find(id);
    if (found == registry.connections.end())
    {
        network::fail("connection_closed", "HTTP 连接已关闭");
    }
    return found->second;
}

std::pair<http_request_data, std::size_t> read_request_head(
    network::tcp_stream& stream, std::size_t max_length)
{
    auto head = network::parse_head(stream.read_head());
    if (!head.cookies.empty())
    {
        network::fail("protocol_error", "HTTP 请求不能包含 Set-Cookie 响应头");
    }
    const auto first = head.first_line.find(' ');
    const auto second = head.first_line.find(' ', first + 1);
    if (first == std::string::npos || second == std::string::npos ||
        head.first_line.find(' ', second + 1) != std::string::npos ||
        head.first_line.substr(second + 1) != "HTTP/1.1")
    {
        network::fail("protocol_error", "仅支持 HTTP/1.1 请求行");
    }
    http_request_data result;
    result.method = head.first_line.substr(0, first);
    network::validate_token(result.method, "HTTP 方法");
    result.target = head.first_line.substr(first + 1, second - first - 1);
    if (result.target.empty() || result.target.front() != '/' ||
        result.target.find_first_of("\r\n\0 ", 0, 4) != std::string::npos)
    {
        network::fail("protocol_error", "HTTP 请求目标无效");
    }
    network::validate_utf8(result.target);
    if (!head.headers.contains("host") ||
        head.headers.contains("transfer-encoding"))
    {
        network::fail("protocol_error", "HTTP/1.1 请求需要 Host 且不支持分块上传");
    }
    const auto length = network::content_length(head.headers, max_length);
    result.headers = std::move(head.headers);
    result.body_length = static_cast<std::int64_t>(length);
    return {std::move(result), length};
}

http_request_data read_request(network::tcp_stream& stream, bool binary)
{
    auto [result, length] = read_request_head(stream, network::max_body_bytes);
    result.body = stream.read_exact(length);
    if (!binary)
    {
        network::validate_utf8(result.body);
    }
    return result;
}

void send_bad_request(network::tcp_stream& stream, const runtime_failure& error)
{
    const auto& code = error.error().code;
    if (code != "protocol_error" && code != "invalid_header" &&
        code != "invalid_utf8" && code != "size_limit")
    {
        return;
    }
    const int status = code == "size_limit"
        ? (error.error().message.find("头") != std::string::npos ? 431 : 413)
        : 400;
    const auto reason = status == 431 ? "Request Header Fields Too Large" :
        status == 413 ? "Payload Too Large" : "Bad Request";
    try
    {
        stream.send_all("HTTP/1.1 " + std::to_string(status) + " " + reason +
                        "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
    }
    catch (...)
    {
        // 保留原始协议错误。
    }
}

std::string reason_phrase(std::int64_t status)
{
    switch (status)
    {
    case 200: return "OK";
    case 201: return "Created";
    case 204: return "No Content";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 304: return "Not Modified";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 413: return "Payload Too Large";
    case 500: return "Internal Server Error";
    case 503: return "Service Unavailable";
    default: return "Status";
    }
}

std::string serialize_response_head(const http_response_data& response,
                                    std::int64_t body_length)
{
    if (response.status < 100 || response.status > 599)
    {
        network::fail("invalid_argument", "HTTP 响应状态码必须在 100～599 之间");
    }
    if ((response.status < 200 || response.status == 204 || response.status == 304) &&
        body_length != 0)
    {
        network::fail("invalid_argument", "此状态码不能携带 HTTP 正文");
    }
    std::string result = "HTTP/1.1 " + std::to_string(response.status) + " " +
        reason_phrase(response.status) + "\r\n";
    for (const auto& [name, value] : response.headers)
    {
        network::validate_header(name, value);
        const auto key = network::lower_ascii(name);
        if (key == "content-length" || key == "transfer-encoding" ||
            key == "connection" || key == "set-cookie")
        {
            network::fail("invalid_header", "响应头中的保留字段需由 httpx 管理");
        }
        result += key + ": " + value + "\r\n";
    }
    for (const auto& cookie : response.cookies)
    {
        network::validate_header("set-cookie", cookie);
        result += "set-cookie: " + cookie + "\r\n";
    }
    result += "Content-Length: " + std::to_string(body_length) +
        "\r\nConnection: close\r\n\r\n";
    if (result.size() > network::max_head_bytes)
    {
        network::fail("size_limit", "HTTP 响应头超过 64 KiB");
    }
    return result;
}

std::string serialize_response(const http_response_data& response, bool binary)
{
    if (response.body.size() > network::max_body_bytes)
    {
        network::fail("size_limit", "HTTP 响应正文超过 8 MiB");
    }
    if (!binary)
    {
        network::validate_utf8(response.body);
    }
    return serialize_response_head(response,
        static_cast<std::int64_t>(response.body.size()));
}

} // namespace

std::int64_t httpx_listen(std::string_view host, std::int64_t port)
{
    auto socket = std::make_shared<network::socket_handle>(
        network::listen_tcp(host, port));
    auto& registry = servers();
    std::lock_guard lock(registry.mutex);
    const auto id = registry.next_id++;
    registry.listeners.emplace(id, std::move(socket));
    return id;
}

std::int64_t httpx_accept(std::int64_t listener, std::int64_t timeout_ms,
                          bool binary)
{
    if (http2::is_http2_id(listener))
    {
        return http2::accept(listener, {}, network::max_body_bytes, binary,
                             timeout_ms);
    }
    auto socket = network::accept_tcp(listener_at(listener)->get(), timeout_ms);
    auto state = std::make_shared<connection_state>(std::move(socket));
    state->stream.set_receive_timeout(timeout_ms);
    try
    {
        state->request = read_request(state->stream, binary);
    }
    catch (const runtime_failure& error)
    {
        send_bad_request(state->stream, error);
        throw;
    }
    auto& registry = servers();
    std::lock_guard lock(registry.mutex);
    const auto id = registry.next_id++;
    registry.connections.emplace(id, std::move(state));
    return id;
}

std::int64_t httpx_accept_stream(std::int64_t listener,
                                 const binary_stream& destination,
                                 std::int64_t max_request_bytes,
                                 std::int64_t timeout_ms)
{
    if (!destination || max_request_bytes < 0)
    {
        network::fail("invalid_argument", "HTTP 请求目标流或长度上限无效");
    }
    destination->file.require_open();
    if (http2::is_http2_id(listener))
    {
        return http2::accept(listener, destination,
            static_cast<std::size_t>(max_request_bytes), true, timeout_ms);
    }
    auto socket = network::accept_tcp(listener_at(listener)->get(), timeout_ms);
    auto state = std::make_shared<connection_state>(std::move(socket));
    state->stream.set_receive_timeout(timeout_ms);
    try
    {
        auto [request, length] = read_request_head(state->stream,
            static_cast<std::size_t>(max_request_bytes));
        state->request = std::move(request);
        constexpr std::size_t block_size = 16 * 1024;
        while (length > 0)
        {
            const auto block = state->stream.read_exact(std::min(length, block_size));
            destination->file.write(block);
            length -= block.size();
        }
    }
    catch (const runtime_failure& error)
    {
        send_bad_request(state->stream, error);
        throw;
    }
    auto& registry = servers();
    std::lock_guard lock(registry.mutex);
    const auto id = registry.next_id++;
    registry.connections.emplace(id, std::move(state));
    return id;
}

http_request_data httpx_request(std::int64_t id)
{
    if (http2::is_http2_id(id))
    {
        return http2::request(id);
    }
    return connection_at(id)->request;
}

void httpx_respond(std::int64_t id, const http_response_data& value,
                   bool binary)
{
    if (http2::is_http2_id(id))
    {
        if (value.body.size() > network::max_body_bytes)
        {
            network::fail("size_limit", "HTTP/2 响应正文超过 8 MiB");
        }
        http2::respond(id, value, {},
            static_cast<std::int64_t>(value.body.size()), binary);
        return;
    }
    auto state = connection_at(id);
    try
    {
        const auto header = serialize_response(value, binary);
        state->stream.send_all(header);
        state->stream.send_all(value.body);
    }
    catch (...)
    {
        httpx_close_connection(id);
        throw;
    }
    httpx_close_connection(id);
}

void httpx_respond_stream(std::int64_t id, const http_response_data& value,
                          const binary_stream& source,
                          std::int64_t body_length)
{
    if (http2::is_http2_id(id))
    {
        http2::respond(id, value, source, body_length, true);
        return;
    }
    if (body_length < 0 || (body_length > 0 && !source))
    {
        network::fail("invalid_argument", "HTTP 响应源流或长度无效");
    }
    auto state = connection_at(id);
    try
    {
        if (body_length > 0)
        {
            source->file.require_open();
        }
        state->stream.send_all(serialize_response_head(value, body_length));
        constexpr std::size_t block_size = 16 * 1024;
        char buffer[block_size];
        auto remaining = static_cast<std::uint64_t>(body_length);
        while (remaining > 0)
        {
            const auto count = source->file.read_into(buffer,
                static_cast<std::size_t>(std::min<std::uint64_t>(remaining,
                                                                block_size)));
            if (count == 0)
            {
                network::fail("operation_failed", "HTTP 响应源流提前结束");
            }
            state->stream.send_all({buffer, count});
            remaining -= count;
        }
    }
    catch (...)
    {
        httpx_close_connection(id);
        throw;
    }
    httpx_close_connection(id);
}

void httpx_close_listener(std::int64_t id) noexcept
{
    if (http2::is_http2_id(id))
    {
        http2::close_listener(id);
        return;
    }
    auto& registry = servers();
    std::lock_guard lock(registry.mutex);
    registry.listeners.erase(id);
}

void httpx_close_connection(std::int64_t id) noexcept
{
    if (http2::is_http2_id(id))
    {
        http2::close_connection(id);
        return;
    }
    auto& registry = servers();
    std::lock_guard lock(registry.mutex);
    registry.connections.erase(id);
}

} // namespace tx_generated
