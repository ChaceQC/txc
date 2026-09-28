#include "stdlib/httpx_server_listener.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <unordered_map>

namespace tx_generated::httpx_listener
{
namespace
{

constexpr std::int64_t id_prefix = std::int64_t{1} << 40;

struct registry
{
    std::mutex mutex;
    std::int64_t next_id = id_prefix;
    std::unordered_map<std::int64_t, std::shared_ptr<listener_state>> listeners;
};

registry& listeners()
{
    static registry result;
    return result;
}

} // namespace

class listener_state : public std::enable_shared_from_this<listener_state>
{
public:
    listener_state(network::socket_handle socket, std::size_t limit)
        : socket_(std::move(socket)), limit_(limit)
    {
    }

    accepted_connection accept(std::int64_t timeout_ms)
    {
        if (timeout_ms < 0 || timeout_ms > std::numeric_limits<int>::max())
        {
            network::fail("invalid_argument", "HTTP 等待超时参数无效");
        }
        const auto deadline = std::chrono::steady_clock::now() +
            std::chrono::milliseconds(timeout_ms);
        acquire_slot(timeout_ms, deadline);
        slot_token slot(shared_from_this());
        std::unique_lock accept_lock(accept_mutex_);
        while (!closed_)
        {
            const auto now = std::chrono::steady_clock::now();
            if (timeout_ms != 0 && now >= deadline)
            {
                network::fail("timeout", "等待 HTTP 连接超时");
            }
            const auto slice = timeout_ms == 0 ? 100 :
                std::min<std::int64_t>(100, std::chrono::duration_cast<
                    std::chrono::milliseconds>(deadline - now).count());
            try
            {
                return {network::accept_tcp(socket_.get(),
                    std::max<std::int64_t>(1, slice)), std::move(slot)};
            }
            catch (const runtime_failure& error)
            {
                if (error.error().code != "timeout" || closed_)
                {
                    throw;
                }
            }
        }
        network::fail("connection_closed", "HTTP 监听器已关闭");
    }

    void release_slot() noexcept
    {
        {
            std::lock_guard lock(capacity_mutex_);
            --active_;
        }
        capacity_changed_.notify_one();
    }

    void close() noexcept
    {
        closed_ = true;
        capacity_changed_.notify_all();
        std::lock_guard lock(accept_mutex_);
        socket_.reset();
    }

private:
    void acquire_slot(std::int64_t timeout_ms,
                      std::chrono::steady_clock::time_point deadline)
    {
        std::unique_lock lock(capacity_mutex_);
        const auto ready = [&]
        {
            return closed_ || active_ < limit_;
        };
        const bool available = timeout_ms == 0
            ? (capacity_changed_.wait(lock, ready), true)
            : capacity_changed_.wait_until(lock, deadline, ready);
        if (!available)
        {
            network::fail("timeout", "HTTP 并发连接已达上限");
        }
        if (closed_)
        {
            network::fail("connection_closed", "HTTP 监听器已关闭");
        }
        ++active_;
    }

    network::socket_handle socket_;
    std::size_t limit_ = 0;
    std::atomic<bool> closed_ = false;
    std::mutex accept_mutex_;
    std::mutex capacity_mutex_;
    std::condition_variable capacity_changed_;
    std::size_t active_ = 0;
};

slot_token::slot_token(std::shared_ptr<listener_state> owner)
    : owner_(std::move(owner))
{
}

slot_token::slot_token(slot_token&& other) noexcept
    : owner_(std::move(other.owner_))
{
}

slot_token::~slot_token()
{
    if (owner_)
    {
        owner_->release_slot();
    }
}

std::int64_t listen(std::string_view host, std::int64_t port,
                    std::int64_t max_connections)
{
    if (max_connections < 1 || max_connections > 64)
    {
        network::fail("invalid_argument", "HTTP 并发连接数须为 1～64");
    }
    auto state = std::make_shared<listener_state>(
        network::listen_tcp(host, port),
        static_cast<std::size_t>(max_connections));
    auto& value = listeners();
    std::lock_guard lock(value.mutex);
    const auto id = value.next_id++;
    value.listeners.emplace(id, std::move(state));
    return id;
}

accepted_connection accept(std::int64_t listener, std::int64_t timeout_ms)
{
    auto& value = listeners();
    std::shared_ptr<listener_state> state;
    {
        std::lock_guard lock(value.mutex);
        const auto found = value.listeners.find(listener);
        if (found == value.listeners.end())
        {
            network::fail("connection_closed", "HTTP 监听器已关闭");
        }
        state = found->second;
    }
    return state->accept(timeout_ms);
}

void close(std::int64_t listener) noexcept
{
    auto& value = listeners();
    std::shared_ptr<listener_state> state;
    {
        std::lock_guard lock(value.mutex);
        const auto found = value.listeners.find(listener);
        if (found == value.listeners.end())
        {
            return;
        }
        state = std::move(found->second);
        value.listeners.erase(found);
    }
    state->close();
}

} // namespace tx_generated::httpx_listener
