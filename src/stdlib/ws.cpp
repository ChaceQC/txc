#include "stdlib/ws.hpp"
#include "stdlib/ws_frames.hpp"

#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

namespace tx_generated
{
namespace
{

struct ws_registry
{
    struct listener_state
    {
        network::socket_handle socket;
        std::optional<tls::server_options> secure;
    };

    std::mutex mutex;
    std::int64_t next_id = 1;
    std::unordered_map<std::int64_t, std::shared_ptr<listener_state>> listeners;
    std::unordered_map<std::int64_t, std::shared_ptr<ws_connection_state>> connections;
};

ws_registry& resources()
{
    static ws_registry result;
    return result;
}

std::shared_ptr<ws_connection_state> connection_at(std::int64_t id)
{
    auto& registry = resources();
    std::lock_guard lock(registry.mutex);
    const auto found = registry.connections.find(id);
    if (found == registry.connections.end())
    {
        network::fail("connection_closed", "WebSocket 连接已关闭");
    }
    return found->second;
}

} // namespace

std::int64_t ws_connect(std::string_view url, std::int64_t timeout_ms)
{
    auto state = ws_client_connect(url, timeout_ms);
    auto& registry = resources();
    std::lock_guard lock(registry.mutex);
    const auto id = registry.next_id++;
    registry.connections.emplace(id, std::move(state));
    return id;
}

std::int64_t ws_listen(std::string_view host, std::int64_t port)
{
    auto state = std::make_shared<ws_registry::listener_state>();
    state->socket = network::listen_tcp(host, port);
    auto& registry = resources();
    std::lock_guard lock(registry.mutex);
    const auto id = registry.next_id++;
    registry.listeners.emplace(id, std::move(state));
    return id;
}

std::int64_t ws_listen_tls(std::string_view host, std::int64_t port,
                           tls::server_options options)
{
    if (options.identity_id == 0)
    {
        network::fail("invalid_argument", "WSS 监听器需要服务端证书身份");
    }
    auto state = std::make_shared<ws_registry::listener_state>();
    state->socket = network::listen_tcp(host, port);
    state->secure = std::move(options);
    auto& registry = resources();
    std::lock_guard lock(registry.mutex);
    const auto id = registry.next_id++;
    registry.listeners.emplace(id, std::move(state));
    return id;
}

std::int64_t ws_accept(std::int64_t listener, std::int64_t timeout_ms)
{
    std::shared_ptr<ws_registry::listener_state> server;
    {
        auto& registry = resources();
        std::lock_guard lock(registry.mutex);
        const auto found = registry.listeners.find(listener);
        if (found == registry.listeners.end())
        {
            network::fail("connection_closed", "WebSocket 监听器已关闭");
        }
        server = found->second;
    }
    auto socket = network::accept_tcp(server->socket.get(), timeout_ms);
    std::shared_ptr<ws_connection_state> state;
    if (server->secure)
    {
        auto native = std::make_shared<network::socket_handle>(
            std::move(socket));
        auto secured = std::make_shared<tls::secure_connection>(
            std::move(native), *server->secure, std::vector<std::string>{},
            timeout_ms == 0 ? 30000 : timeout_ms);
        state = ws_server_accept(std::make_unique<network::tcp_stream>(
            std::move(secured)), timeout_ms);
    }
    else
    {
        state = ws_server_accept(std::move(socket), timeout_ms);
    }
    auto& registry = resources();
    std::lock_guard lock(registry.mutex);
    const auto id = registry.next_id++;
    registry.connections.emplace(id, std::move(state));
    return id;
}

std::int64_t ws_upgrade_http(std::int64_t connection)
{
    auto state = ws_server_upgrade(httpx_take_upgrade_connection(connection));
    auto& registry = resources();
    std::lock_guard lock(registry.mutex);
    const auto id = registry.next_id++;
    registry.connections.emplace(id, std::move(state));
    return id;
}

void send_message(std::int64_t id, std::string_view data, bool binary)
{
    auto state = connection_at(id);
    if (!state->open)
    {
        network::fail("connection_closed", "WebSocket 连接已关闭");
    }
    if (!binary)
    {
        network::validate_utf8(data);
    }
    if (data.size() > network::max_body_bytes)
    {
        network::fail("size_limit", "WebSocket 文本消息超过 8 MiB");
    }
    if (state->server)
    {
        ws_server_send(*state, data, binary);
    }
    else
    {
        ws_client_send(*state, data, binary);
    }
}

void ws_send_text(std::int64_t id, std::string_view text)
{
    send_message(id, text, false);
}

void ws_send_binary(std::int64_t id, std::string_view data)
{
    send_message(id, data, true);
}

void ws_send_binary_stream(std::int64_t id, const binary_stream& source,
                           std::int64_t length)
{
    if (length < 0 || (length > 0 && !source))
    {
        network::fail("invalid_argument", "WebSocket 消息源流或长度无效");
    }
    auto state = connection_at(id);
    if (!state->open)
    {
        network::fail("connection_closed", "WebSocket 连接已关闭");
    }
    if (length > 0)
    {
        source->file.require_open();
    }
    try
    {
        if (state->server)
        {
            ws_server_send_stream(*state, source, length);
        }
        else
        {
            ws_client_send_stream(*state, source, length);
        }
    }
    catch (...)
    {
        state->open = false;
        ws_close_connection(id);
        throw;
    }
}

ws_message_data receive_message(std::int64_t id, std::int64_t timeout_ms,
                                bool binary)
{
    auto state = connection_at(id);
    if (!state->open)
    {
        network::fail("connection_closed", "WebSocket 连接已关闭");
    }
    if (timeout_ms < 0)
    {
        network::fail("invalid_argument", "WebSocket 读取超时不能为负");
    }
    auto value = state->server ? ws_server_receive(*state, timeout_ms, binary)
                               : ws_client_receive(state, timeout_ms, binary);
    state->open = value.open;
    return value;
}

ws_message_data ws_receive(std::int64_t id, std::int64_t timeout_ms)
{
    return receive_message(id, timeout_ms, false);
}

ws_message_data ws_receive_binary(std::int64_t id, std::int64_t timeout_ms)
{
    return receive_message(id, timeout_ms, true);
}

ws_stream_message_data ws_receive_binary_stream(
    std::int64_t id, const binary_stream& destination,
    std::int64_t max_message_bytes, std::int64_t timeout_ms)
{
    if (!destination || max_message_bytes < 0 || timeout_ms < 0)
    {
        network::fail("invalid_argument", "WebSocket 目标流、长度上限或超时无效");
    }
    destination->file.require_open();
    auto state = connection_at(id);
    if (!state->open)
    {
        network::fail("connection_closed", "WebSocket 连接已关闭");
    }
    try
    {
        auto result = state->server
            ? ws_server_receive_stream(*state, destination, max_message_bytes,
                                       timeout_ms)
            : ws_client_receive_stream(state, destination, max_message_bytes,
                                       timeout_ms);
        state->open = result.open;
        if (!result.open)
        {
            state->close_status = {true, result.close_code,
                result.close_reason};
        }
        return result;
    }
    catch (...)
    {
        state->open = false;
        ws_close_connection(id);
        throw;
    }
}

bool ws_is_open(std::int64_t id)
{
    return connection_at(id)->open;
}

ws_close_status_data ws_close_status(std::int64_t id)
{
    return connection_at(id)->close_status;
}

void send_control(std::int64_t id, std::string_view data,
                  std::uint8_t opcode)
{
    auto state = connection_at(id);
    if (!state->open)
    {
        network::fail("connection_closed", "WebSocket 连接已关闭");
    }
    if (!state->server)
    {
        network::fail("unsupported_option", "WinHTTP 客户端由系统处理 ping/pong");
    }
    if (data.size() > 125)
    {
        network::fail("size_limit", "WebSocket 控制帧正文超过 125 字节");
    }
    network::send_ws_frame(*state->stream, opcode, data);
}

void ws_send_ping(std::int64_t id, std::string_view data)
{
    send_control(id, data, 0x9);
}

void ws_send_pong(std::int64_t id, std::string_view data)
{
    send_control(id, data, 0xA);
}

void ws_close_with_reason(std::int64_t id, std::int64_t code,
                          std::string_view reason)
{
    if (code < 1000 || code > 4999 || code == 1004 || code == 1005 ||
        code == 1006 || code == 1015 ||
        (code >= 1016 && code < 3000) || reason.size() > 123)
    {
        network::fail("invalid_argument", "WebSocket 关闭码或原因长度无效");
    }
    network::validate_utf8(reason);
    auto state = connection_at(id);
    if (state->server)
    {
        ws_server_close(*state, static_cast<std::uint16_t>(code), reason);
    }
    else
    {
        ws_client_close(*state, static_cast<std::uint16_t>(code), reason);
    }
    auto& registry = resources();
    std::lock_guard lock(registry.mutex);
    registry.connections.erase(id);
}

void ws_close_listener(std::int64_t id) noexcept
{
    auto& registry = resources();
    std::lock_guard lock(registry.mutex);
    registry.listeners.erase(id);
}

void ws_close_connection(std::int64_t id) noexcept
{
    std::shared_ptr<ws_connection_state> state;
    {
        auto& registry = resources();
        std::lock_guard lock(registry.mutex);
        const auto found = registry.connections.find(id);
        if (found == registry.connections.end())
        {
            return;
        }
        state = std::move(found->second);
        registry.connections.erase(found);
    }
    if (state->server)
    {
        ws_server_close(*state);
    }
    else
    {
        ws_client_close(*state);
    }
    state->open = false;
}

} // namespace tx_generated
