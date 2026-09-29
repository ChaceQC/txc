#include "stdlib/format_plan_cache.hpp"

#include <list>

namespace tx_generated
{
namespace
{

struct cache_entry
{
    std::shared_ptr<const dynamic_format_plan> plan;
    std::size_t bytes;
};

struct plan_cache
{
    std::list<cache_entry> entries;
    std::size_t bytes = 0;
};

// 每线程独立 LRU，无共享锁；调用者持有不可变计划，重入淘汰不会使当前计划悬空。
thread_local plan_cache cache;

std::size_t plan_bytes(const dynamic_format_plan& plan)
{
    std::size_t bytes = sizeof(dynamic_format_plan) + sizeof(cache_entry) + 4 * sizeof(void*) +
        plan.text.capacity() + 1 + plan.parts.capacity() * sizeof(dynamic_format_part);
    for (const auto& part : plan.parts)
    {
        bytes += part.literal.capacity() + part.name.capacity() + 2;
    }
    return bytes;
}

} // namespace

std::shared_ptr<const dynamic_format_plan> find_format_plan(std::string_view text)
{
    for (auto entry = cache.entries.begin(); entry != cache.entries.end(); ++entry)
    {
        // 唯一模式为公开 format 语义；完整内容参与比较，不以地址或哈希作为身份。
        if (entry->plan->text == text)
        {
            auto plan = entry->plan;
            cache.entries.splice(cache.entries.begin(), cache.entries, entry);
            return plan;
        }
    }
    return {};
}

void remember_format_plan(dynamic_format_plan plan)
{
    const auto bytes = plan_bytes(plan);
    if (plan.text.size() > format_cache_template_bytes || bytes > format_cache_bytes ||
        find_format_plan(plan.text))
    {
        return;
    }
    // 缓存为可选加速；分配失败不把已经成功的格式化变为错误。
    try
    {
        auto saved = std::make_shared<const dynamic_format_plan>(std::move(plan));
        while (!cache.entries.empty() && (cache.entries.size() >= format_cache_entries ||
            cache.bytes > format_cache_bytes - bytes))
        {
            cache.bytes -= cache.entries.back().bytes;
            cache.entries.pop_back();
        }
        cache.entries.push_front({std::move(saved), bytes});
        cache.bytes += bytes;
    }
    catch (const std::bad_alloc&)
    {
    }
}

} // namespace tx_generated
