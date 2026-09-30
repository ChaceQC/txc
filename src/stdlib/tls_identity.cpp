#include "stdlib/tls.hpp"

#include "stdlib/error.hpp"
#include "stdlib/x509.hpp"

#include <limits>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace tx_generated::tls
{
namespace
{

struct identity_registry
{
    std::mutex mutex;
    std::int64_t next_id = 1;
    std::unordered_map<std::int64_t, std::shared_ptr<identity_state>> values;
};

identity_registry& registry()
{
    static identity_registry result;
    return result;
}

} // namespace

std::int64_t import_identity(const byte_value& package,
                             const secret::handle& password)
{
    auto certificates = x509::parse_pkcs12(package, password);
    auto private_key = x509::pkcs12_private_key(package, password);
    if (certificates.empty())
    {
        throw runtime_failure({tx::error_kind::security, "invalid_pkcs12",
            "客户端或服务端身份缺少证书"});
    }

    auto state = std::make_shared<identity_state>();
    state->certificates = std::move(certificates);
    state->private_key = std::move(private_key);
    auto& records = registry();
    std::lock_guard lock(records.mutex);
    if (records.next_id == std::numeric_limits<std::int64_t>::max())
    {
        throw runtime_failure({tx::error_kind::security, "size_limit",
            "TLS 身份编号已耗尽"});
    }
    const auto id = records.next_id;
    records.values.emplace(id, std::move(state));
    ++records.next_id;
    return id;
}

std::shared_ptr<const identity_state> get_identity(std::int64_t id)
{
    auto& records = registry();
    std::lock_guard lock(records.mutex);
    const auto found = records.values.find(id);
    if (found == records.values.end())
    {
        throw runtime_failure({tx::error_kind::security, "invalid_state",
            "TLS 身份不存在或已关闭"});
    }
    return found->second;
}

void close_identity(std::int64_t id) noexcept
{
    if (id == 0)
    {
        return;
    }
    auto& records = registry();
    std::lock_guard lock(records.mutex);
    const auto found = records.values.find(id);
    if (found != records.values.end())
    {
        {
            // 身份对象仍由登记表持有时释放锁，避免删除对象后解锁悬空互斥锁。
            std::lock_guard identity_lock(found->second->mutex);
            found->second->closed = true;
            found->second->available_materials.clear();
            secret::close(found->second->private_key);
        }
        records.values.erase(found);
    }
}

} // namespace tx_generated::tls
