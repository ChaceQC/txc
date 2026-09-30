#include "stdlib/x509_verification_context.hpp"

#ifdef _WIN32
#include <algorithm>
#include <list>

namespace tx_generated::x509
{
namespace
{

void add_certificate(HCERTSTORE store, const byte_value& data)
{
    const auto parsed = certificate(data);
    if (!CertAddCertificateContextToStore(store, parsed.get(),
            CERT_STORE_ADD_USE_EXISTING, nullptr))
    {
        fail("operation_failed", "添加证书到验证存储失败");
    }
}

void add_system_roots(HCERTSTORE roots, DWORD location)
{
    store_ptr system(CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
        location | CERT_STORE_READONLY_FLAG, L"ROOT"));
    if (!system)
    {
        fail("operation_failed", "读取系统根证书存储失败");
    }
    certificate_cursor cursor;
    while (const auto* current = cursor.next(system.get()))
    {
        if (!CertAddCertificateContextToStore(roots, current,
                CERT_STORE_ADD_USE_EXISTING, nullptr))
        {
            fail("operation_failed", "加入系统信任锚失败");
        }
    }
}

std::shared_ptr<verification_context> create_context(const byte_value& leaf,
    const bytes_vector& intermediates, const bytes_vector& anchors, bool system_trust)
{
    auto result = std::make_shared<verification_context>();
    result->leaf_bytes = leaf;
    result->intermediate_bytes = intermediates.data().values;
    result->anchor_bytes = anchors.data().values;
    result->leaf = certificate(leaf);
    result->additional = memory_store();
    for (const auto& item : result->intermediate_bytes)
    {
        add_certificate(result->additional.get(), item);
    }
    result->roots = memory_store();
    for (const auto& item : result->anchor_bytes)
    {
        add_certificate(result->roots.get(), item);
    }
    if (system_trust)
    {
        add_system_roots(result->roots.get(), CERT_SYSTEM_STORE_CURRENT_USER);
        add_system_roots(result->roots.get(), CERT_SYSTEM_STORE_LOCAL_MACHINE);
    }
    CERT_CHAIN_ENGINE_CONFIG config{};
    config.cbSize = sizeof(config);
    // 中间证书仅辅助建链；只有显式根和本轮系统根具有信任资格。
    config.hExclusiveRoot = result->roots.get();
    HCERTCHAINENGINE raw = nullptr;
    if (!CertCreateCertificateChainEngine(&config, &raw))
    {
        fail("operation_failed", "创建证书链引擎失败");
    }
    result->engine.reset(raw);
    return result;
}

bool same_bytes(const byte_value& left, const byte_value& right)
{
    return left == right || (left && right && left->size() == right->size() &&
        std::equal(left->begin(), left->end(), right->begin()));
}

bool matches(const verification_context& entry, const byte_value& leaf,
    const bytes_vector& intermediates, const bytes_vector& anchors)
{
    return same_bytes(entry.leaf_bytes, leaf) &&
        std::equal(entry.intermediate_bytes.begin(), entry.intermediate_bytes.end(),
            intermediates.data().values.begin(), intermediates.data().values.end(), same_bytes) &&
        std::equal(entry.anchor_bytes.begin(), entry.anchor_bytes.end(),
            anchors.data().values.begin(), anchors.data().values.end(), same_bytes);
}

struct context_cache
{
    std::mutex mutex;
    std::list<std::shared_ptr<verification_context>> entries;
};

} // namespace

std::shared_ptr<verification_context> acquire_verification_context(
    const byte_value& leaf, const bytes_vector& intermediates,
    const bytes_vector& anchors, bool system_trust)
{
    // 系统信任存储会在进程外变化，每次重读。自定义信任只缓存完全相同的
    // 叶证书、中间证书和锚；不让其他调用的中间证书缓存改变本轮建链输入。
    std::size_t bytes = leaf ? leaf->size() : 0;
    for (const auto& item : intermediates.data().values)
    {
        bytes += item ? item->size() : 0;
    }
    for (const auto& item : anchors.data().values)
    {
        bytes += item ? item->size() : 0;
    }
    if (system_trust || bytes > max_certificate_bytes)
    {
        return create_context(leaf, intermediates, anchors, system_trust);
    }
    static context_cache cache;
    const auto find = [&]() -> std::shared_ptr<verification_context>
    {
        for (auto item = cache.entries.begin(); item != cache.entries.end(); ++item)
        {
            if (matches(**item, leaf, intermediates, anchors))
            {
                auto result = *item;
                cache.entries.splice(cache.entries.begin(), cache.entries, item);
                return result;
            }
        }
        return {};
    };
    {
        std::lock_guard lock(cache.mutex);
        if (auto result = find())
        {
            return result;
        }
    }
    auto result = create_context(leaf, intermediates, anchors, false);
    std::lock_guard lock(cache.mutex);
    if (auto existing = find())
    {
        return existing;
    }
    // 最多四组、每组最多 1 MiB 输入；活动验证持有独立 shared_ptr。
    cache.entries.push_front(result);
    if (cache.entries.size() > 4)
    {
        cache.entries.pop_back();
    }
    return result;
}

} // namespace tx_generated::x509
#endif
