#pragma once

#include "stdlib/format_arguments.hpp"
#include "stdlib/format_plan_cache.hpp"

#include <list>

namespace tx_generated
{

inline constexpr std::size_t format_binding_entries = 16;
inline constexpr std::size_t format_binding_arguments = 128;
inline constexpr std::size_t format_binding_parts = 128;

struct format_binding_slot
{
    std::size_t index;
    bool keyword;
};

struct format_binding_plan
{
    std::shared_ptr<const dynamic_format_plan> syntax;
    std::size_t positional;
    std::vector<std::string> keywords;
    std::vector<format_binding_slot> slots;
};

// 绑定仅依赖位置数量和有序关键字名称。规格与类型兼容性仍逐次检查，
// 不缓存值或地址；每线程至多 16 项，每项总共至多 128 个参数、128 个计划片段。
extern thread_local std::list<std::shared_ptr<const format_binding_plan>> format_bindings;

inline bool cacheable_format_binding(const dynamic_format_plan& plan,
    const direct_format_arguments& args)
{
    if (args.positional.size() > format_binding_arguments ||
        args.keywords.size() > format_binding_arguments - args.positional.size() ||
        plan.parts.size() > format_binding_parts)
    {
        return false;
    }
    std::size_t bytes = 0;
    for (const auto& argument : args.keywords)
    {
        const std::string_view name(argument.name);
        if (name.size() > format_cache_template_bytes - bytes)
        {
            return false;
        }
        bytes += name.size();
    }
    return true;
}

inline std::shared_ptr<const format_binding_plan> find_format_binding(
    const std::shared_ptr<const dynamic_format_plan>& syntax, const direct_format_arguments& args)
{
    for (auto item = format_bindings.begin(); item != format_bindings.end(); ++item)
    {
        const auto& plan = **item;
        if (plan.syntax != syntax || plan.positional != args.size() ||
            plan.keywords.size() != args.keywords.size())
        {
            continue;
        }
        bool matches = true;
        for (std::size_t index = 0; index < plan.keywords.size(); ++index)
        {
            matches &= plan.keywords[index] == args.keywords[index].name;
        }
        if (matches)
        {
            auto result = *item;
            format_bindings.splice(format_bindings.begin(), format_bindings, item);
            return result;
        }
    }
    return {};
}

inline format_binding_slot locate_format_argument(const direct_format_arguments& args,
    const format_argument& value)
{
    for (std::size_t index = 0; index < args.keywords.size(); ++index)
    {
        if (&args.keywords[index] == &value)
        {
            return {index, true};
        }
    }
    return {static_cast<std::size_t>(&value - args.positional.data()), false};
}

inline void remember_format_binding(const std::shared_ptr<const dynamic_format_plan>& syntax,
    const direct_format_arguments& args, std::vector<format_binding_slot> slots)
{
    try
    {
        format_binding_plan plan{syntax, args.size(), {}, std::move(slots)};
        for (const auto& argument : args.keywords)
        {
            plan.keywords.emplace_back(argument.name);
        }
        format_bindings.push_front(std::make_shared<const format_binding_plan>(std::move(plan)));
        if (format_bindings.size() > format_binding_entries)
        {
            format_bindings.pop_back();
        }
    }
    catch (const std::bad_alloc&)
    {
        // 缓存失败不改变已经成功的格式化结果。
    }
}

} // namespace tx_generated
