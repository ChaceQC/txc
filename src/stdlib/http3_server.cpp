#include "stdlib/http3_server_internal.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace tx_generated::http3
{
namespace
{

constexpr std::int64_t id_prefix = std::int64_t{1} << 61;

struct registry
{
    std::mutex mutex;
    std::int64_t next_id = id_prefix;
    std::unordered_map<std::int64_t, std::shared_ptr<server_listener>> listeners;
    std::unordered_map<std::int64_t, server_request_ticket> requests;
};

registry& states()
{
    static registry result;
    return result;
}

std::shared_ptr<server_listener> listener_at(std::int64_t id)
{
    auto& value = states();
    std::lock_guard lock(value.mutex);
    const auto found = value.listeners.find(id);
    if (found == value.listeners.end())
    {
        network::fail("connection_closed", "HTTP/3 监听器已关闭");
    }
    return found->second;
}

server_request_ticket request_at(std::int64_t id)
{
    auto& value = states();
    std::lock_guard lock(value.mutex);
    const auto found = value.requests.find(id);
    if (found == value.requests.end())
    {
        network::fail("connection_closed", "HTTP/3 请求流已关闭");
    }
    return found->second;
}

} // namespace

bool is_http3_id(std::int64_t id) noexcept
{
    return id >= id_prefix;
}

std::int64_t listen(std::string_view host, std::int64_t port,
                    const byte_value& package,
                    const secret::handle& password)
{
    auto state = std::make_shared<server_listener>(host, port,
                                                    package, password);
    auto& value = states();
    std::lock_guard lock(value.mutex);
    const auto id = value.next_id++;
    value.listeners.emplace(id, std::move(state));
    return id;
}

std::int64_t accept(std::int64_t listener, const binary_stream& destination,
                    std::size_t max_request_bytes, bool binary,
                    std::int64_t timeout_ms)
{
    if (max_request_bytes > network::max_body_bytes)
    {
        network::fail("invalid_argument", "HTTP/3 接收流或长度上限无效");
    }
    if (destination)
    {
        destination->file.require_open();
    }
    auto owner = listener_at(listener);
    auto pending = owner->pop(timeout_ms);
    auto& request = *pending.request;
    try
    {
        if (request.value.body.size() > max_request_bytes)
        {
            network::fail("size_limit", "HTTP/3 请求正文超过接收上限");
        }
        if (destination)
        {
            destination->file.write(request.value.body);
            request.value.body.clear();
        }
        else if (!binary)
        {
            network::validate_utf8(request.value.body);
        }
    }
    catch (...)
    {
        pending.connection->close_stream(request.stream_id);
        throw;
    }
    if (!pending.connection->mark_delivered(pending.request))
    {
        pending.connection->close_stream(request.stream_id);
        network::fail("connection_closed", "HTTP/3 请求流已关闭");
    }
    pending.listener = std::move(owner);
    try
    {
        auto& value = states();
        std::lock_guard lock(value.mutex);
        const auto id = value.next_id++;
        value.requests.emplace(id, pending);
        return id;
    }
    catch (...)
    {
        pending.connection->close_stream(request.stream_id);
        throw;
    }
}

http_request_data request(std::int64_t id)
{
    return request_at(id).request->value;
}

void respond(std::int64_t id, const http_response_data& value,
             const binary_stream& source, std::int64_t body_length,
             bool binary)
{
    auto pending = request_at(id);
    try
    {
        if (body_length < 0 || body_length >
            static_cast<std::int64_t>(network::max_body_bytes))
        {
            network::fail("size_limit", "HTTP/3 响应正文超过 8 MiB");
        }
        auto response = value;
        if (source)
        {
            source->file.require_open();
            constexpr std::size_t block_size = 16 * 1024;
            char buffer[block_size];
            auto remaining = static_cast<std::size_t>(body_length);
            response.body.clear();
            response.body.reserve(remaining);
            while (remaining > 0)
            {
                const auto amount = source->file.read_into(buffer,
                    std::min(remaining, block_size));
                if (amount == 0)
                {
                    network::fail("operation_failed",
                                  "HTTP/3 响应源流提前结束");
                }
                response.body.append(buffer, amount);
                remaining -= amount;
            }
        }
        else if (static_cast<std::int64_t>(response.body.size()) != body_length)
        {
            network::fail("invalid_argument", "HTTP/3 响应正文长度不匹配");
        }
        if (!binary)
        {
            network::validate_utf8(response.body);
        }
        pending.connection->respond(pending.request, response);
    }
    catch (...)
    {
        close_connection(id);
        throw;
    }
    auto& registry = states();
    std::lock_guard lock(registry.mutex);
    registry.requests.erase(id);
}

void close_listener(std::int64_t id) noexcept
{
    std::shared_ptr<server_listener> state;
    {
        auto& value = states();
        std::lock_guard lock(value.mutex);
        const auto found = value.listeners.find(id);
        if (found == value.listeners.end())
        {
            return;
        }
        state = std::move(found->second);
        value.listeners.erase(found);
    }
    state->close();
}

void close_connection(std::int64_t id) noexcept
{
    server_request_ticket state;
    {
        auto& value = states();
        std::lock_guard lock(value.mutex);
        const auto found = value.requests.find(id);
        if (found == value.requests.end())
        {
            return;
        }
        state = std::move(found->second);
        value.requests.erase(found);
    }
    state.connection->close_stream(state.request->stream_id);
}

} // namespace tx_generated::http3
