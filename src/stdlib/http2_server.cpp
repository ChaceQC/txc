#include "stdlib/http2_server.hpp"

#include "stdlib/http2_session.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace tx_generated::http2
{
namespace
{

constexpr std::int64_t id_prefix = std::int64_t{1} << 60;

struct listener_state
{
    listener_state(network::socket_handle socket,
                   std::shared_ptr<tls_config> credentials)
        : socket(std::move(socket)), credentials(std::move(credentials))
    {
    }
    network::socket_handle socket;
    std::shared_ptr<tls_config> credentials;
    std::shared_ptr<server_session> active;
};

struct connection_state
{
    std::shared_ptr<server_session> session;
    std::shared_ptr<request_state> stream;
};

struct server_registry
{
    std::mutex mutex;
    std::int64_t next_id = id_prefix;
    std::unordered_map<std::int64_t, std::shared_ptr<listener_state>> listeners;
    std::unordered_map<std::int64_t, connection_state> connections;
};

server_registry& servers()
{
    static server_registry result;
    return result;
}

std::shared_ptr<listener_state> listener_at(std::int64_t id)
{
    auto& registry = servers();
    std::lock_guard lock(registry.mutex);
    const auto found = registry.listeners.find(id);
    if (found == registry.listeners.end())
    {
        network::fail("connection_closed", "HTTP/2 监听器已关闭");
    }
    return found->second;
}

connection_state connection_at(std::int64_t id)
{
    auto& registry = servers();
    std::lock_guard lock(registry.mutex);
    const auto found = registry.connections.find(id);
    if (found == registry.connections.end())
    {
        network::fail("connection_closed", "HTTP/2 请求流已关闭");
    }
    return found->second;
}

std::int64_t register_listener(std::shared_ptr<listener_state> state)
{
    auto& registry = servers();
    std::lock_guard lock(registry.mutex);
    const auto id = registry.next_id++;
    registry.listeners.emplace(id, std::move(state));
    return id;
}

} // namespace

bool is_http2_id(std::int64_t id) noexcept
{
    return id >= id_prefix;
}

std::int64_t listen_h2c(std::string_view host, std::int64_t port)
{
    return register_listener(std::make_shared<listener_state>(
        network::listen_tcp(host, port), nullptr));
}

std::int64_t listen_h2_tls(std::string_view host, std::int64_t port,
                           std::string_view cert_pem,
                           std::string_view key_pem)
{
    auto credentials = std::make_shared<tls_config>(cert_pem, key_pem);
    return register_listener(std::make_shared<listener_state>(
        network::listen_tcp(host, port), std::move(credentials)));
}

std::int64_t accept(std::int64_t listener, const binary_stream& destination,
                    std::size_t max_request_bytes, bool binary,
                    std::int64_t timeout_ms)
{
    auto state = listener_at(listener);
    while (true)
    {
        if (!state->active)
        {
            auto socket = network::accept_tcp(state->socket.get(), timeout_ms);
            state->active = std::make_shared<server_session>(
                std::move(socket), state->credentials, timeout_ms);
        }
        auto stream = state->active->accept(destination, max_request_bytes,
                                            binary, timeout_ms);
        if (!stream)
        {
            state->active.reset();
            continue;
        }
        auto& registry = servers();
        std::lock_guard lock(registry.mutex);
        const auto id = registry.next_id++;
        registry.connections.emplace(id, connection_state{state->active,
                                                            std::move(stream)});
        return id;
    }
}

http_request_data request(std::int64_t id)
{
    return connection_at(id).stream->request;
}

void respond(std::int64_t id, const http_response_data& value,
             const binary_stream& source, std::int64_t body_length,
             bool binary)
{
    auto state = connection_at(id);
    try
    {
        state.session->respond(state.stream, value, source, body_length, binary);
    }
    catch (...)
    {
        close_connection(id);
        throw;
    }
    auto& registry = servers();
    std::lock_guard lock(registry.mutex);
    registry.connections.erase(id);
}

void close_listener(std::int64_t id) noexcept
{
    auto& registry = servers();
    std::lock_guard lock(registry.mutex);
    registry.listeners.erase(id);
}

void close_connection(std::int64_t id) noexcept
{
    connection_state state;
    {
        auto& registry = servers();
        std::lock_guard lock(registry.mutex);
        const auto found = registry.connections.find(id);
        if (found == registry.connections.end())
        {
            return;
        }
        state = std::move(found->second);
        registry.connections.erase(found);
    }
    state.session->close_stream(state.stream);
}

} // namespace tx_generated::http2
