#include "stdlib/http2_server.hpp"

#include "stdlib/http2_session.hpp"
#include "stdlib/error.hpp"

#include <atomic>
#include <chrono>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

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
    std::mutex accept_mutex;
    std::vector<std::shared_ptr<server_session>> sessions;
    std::atomic<bool> closed = false;
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

void remove_session_connections(
    const std::shared_ptr<server_session>& session) noexcept
{
    auto& registry = servers();
    std::lock_guard lock(registry.mutex);
    std::erase_if(registry.connections, [&](const auto& entry)
    {
        return entry.second.session == session;
    });
}

void retire_session(listener_state& listener,
                    std::vector<std::shared_ptr<server_session>>::iterator& item)
{
    auto session = *item;
    session->close();
    item = listener.sessions.erase(item);
    remove_session_connections(session);
}

void retire_closed_sessions(listener_state& listener)
{
    for (auto item = listener.sessions.begin(); item != listener.sessions.end();)
    {
        if ((*item)->closed())
        {
            retire_session(listener, item);
        }
        else
        {
            ++item;
        }
    }
}

} // namespace

bool is_http2_id(std::int64_t id) noexcept
{
    return id >= id_prefix && id < (std::int64_t{1} << 61);
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
    if (timeout_ms < 0 || timeout_ms > std::numeric_limits<int>::max())
    {
        network::fail("invalid_argument", "HTTP/2 等待超时参数无效");
    }
    auto state = listener_at(listener);
    std::lock_guard accept_lock(state->accept_mutex);
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    while (true)
    {
        if (state->closed)
        {
            retire_closed_sessions(*state);
            network::fail("connection_closed", "HTTP/2 监听器已关闭");
        }
        for (auto item = state->sessions.begin(); item != state->sessions.end();)
        {
            if ((*item)->closed())
            {
                retire_session(*state, item);
                continue;
            }
            std::shared_ptr<request_state> stream;
            try
            {
                stream = (*item)->accept(destination, max_request_bytes,
                                         binary, 1);
            }
            catch (const runtime_failure& error)
            {
                if (error.error().code == "timeout" && !(*item)->closed())
                {
                    ++item;
                    continue;
                }
                if ((*item)->closed())
                {
                    retire_session(*state, item);
                }
                throw;
            }
            catch (...)
            {
                if ((*item)->closed())
                {
                    retire_session(*state, item);
                }
                throw;
            }
            if (!stream)
            {
                retire_session(*state, item);
                continue;
            }
            auto& registry = servers();
            std::lock_guard lock(registry.mutex);
            const auto id = registry.next_id++;
            registry.connections.emplace(id, connection_state{*item,
                                                                std::move(stream)});
            return id;
        }
        if (state->sessions.size() < 16)
        {
            fd_set ready;
            FD_ZERO(&ready);
            FD_SET(state->socket.get(), &ready);
            timeval wait{0, 0};
            const int selected = select(0, &ready, nullptr, nullptr, &wait);
            if (selected < 0)
            {
                network::socket_failure("等待 HTTP/2 连接");
            }
            if (selected > 0)
            {
                auto socket = network::accept_tcp(state->socket.get(), 1);
                state->sessions.push_back(std::make_shared<server_session>(
                    std::move(socket), state->credentials, 5000));
                continue;
            }
        }
        if (timeout_ms != 0 && std::chrono::steady_clock::now() >= deadline)
        {
            network::fail("timeout", "等待 HTTP/2 请求超时");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
    std::shared_ptr<listener_state> state;
    auto& registry = servers();
    {
        std::lock_guard lock(registry.mutex);
        const auto found = registry.listeners.find(id);
        if (found == registry.listeners.end())
        {
            return;
        }
        state = found->second;
        state->closed = true;
        registry.listeners.erase(found);
    }
    std::unique_lock accept_lock(state->accept_mutex, std::try_to_lock);
    if (accept_lock.owns_lock())
    {
        retire_closed_sessions(*state);
    }
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
