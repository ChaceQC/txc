#include "stdlib/tls_stream.hpp"

#include "stdlib/error.hpp"

#include <limits>
#include <iterator>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace tx_generated::tls
{
namespace
{

struct registry
{
    std::mutex mutex;
    std::int64_t next_id = 1;
    std::unordered_map<std::int64_t, std::weak_ptr<secure_connection>> values;
};

registry& records()
{
    static registry value;
    return value;
}

} // namespace

stream_resource register_stream(std::shared_ptr<secure_connection> value)
{
    auto& registry = records();
    std::lock_guard lock(registry.mutex);
    if (registry.next_id == std::numeric_limits<std::int64_t>::max())
    {
        network::fail("size_limit", "TLS 流句柄编号已耗尽");
    }
    const auto id = registry.next_id++;
    if (id % 256 == 0)
    {
        for (auto item = registry.values.begin();
             item != registry.values.end();)
        {
            item = item->second.expired()
                ? registry.values.erase(item) : std::next(item);
        }
    }
    registry.values.emplace(id, value);
    return {id, std::move(value)};
}

std::shared_ptr<secure_connection> get_stream(std::int64_t id)
{
    std::shared_ptr<secure_connection> value;
    {
        auto& registry = records();
        std::lock_guard lock(registry.mutex);
        if (const auto found = registry.values.find(id);
            found != registry.values.end())
        {
            value = found->second.lock();
        }
    }
    if (!value || value->closed())
    {
        network::fail("connection_closed", "TLS 安全流已关闭");
    }
    return value;
}

void close_stream(std::int64_t id, std::int64_t timeout_ms)
{
    std::shared_ptr<secure_connection> value;
    {
        auto& registry = records();
        std::lock_guard lock(registry.mutex);
        if (const auto found = registry.values.find(id);
            found != registry.values.end())
        {
            value = found->second.lock();
            registry.values.erase(found);
        }
    }
    if (value)
    {
        value->close(timeout_ms);
    }
}

} // namespace tx_generated::tls
