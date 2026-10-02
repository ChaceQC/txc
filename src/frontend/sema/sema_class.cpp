#include "frontend/sema/sema.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <unordered_set>

namespace tx
{

void semantic_analyzer::index_classes(program& source)
{
    classes_.clear();
    source.field_slot_count = 0;
    source.virtual_slot_count = 0;
    std::vector<std::pair<std::string, class_decl*>> precompiled;
    std::vector<class_decl*> application;
    for (auto& definition : source.classes)
    {
        if (!classes_.emplace(definition.name, &definition).second)
        {
            throw compile_error(definition.position,
                                "重复的类名：" + definition.source_name);
        }
        std::string layout_key;
        for (const auto& method : definition.methods)
        {
            if (method.external_name.starts_with("requests.response.") ||
                method.external_name.starts_with("requests.session."))
            {
                layout_key = method.external_name.substr(0, method.external_name.find_last_of('.'));
                break;
            }
        }
        if (layout_key.empty())
        {
            application.push_back(&definition);
        }
        else
        {
            precompiled.emplace_back(std::move(layout_key), &definition);
        }
    }
    // Requests 的对象来自独立编译的桥接库。先按固定顺序登记其字段，
    // 再分配应用字段，保证库与调用方不受各自模块导入顺序影响。
    std::sort(precompiled.begin(), precompiled.end(), [](const auto& left, const auto& right)
    {
        return left.first < right.first;
    });
    const auto assign_fields = [&](class_decl& definition)
    {
        for (auto& field : definition.fields)
        {
            field.slot = source.field_slot_count++;
        }
    };
    for (const auto& [key, definition] : precompiled)
    {
        (void)key;
        assign_fields(*definition);
    }
    for (auto* definition : application)
    {
        assign_fields(*definition);
    }
}

std::size_t semantic_analyzer::class_distance(
    const value_type& actual, const value_type& expected) const
{
    if (!classes_.contains(actual.name) || !classes_.contains(expected.name))
    {
        return std::numeric_limits<std::size_t>::max();
    }
    std::queue<std::pair<std::string, std::size_t>> pending;
    std::unordered_set<std::string> visited;
    pending.emplace(actual.name, 0);
    visited.insert(actual.name);
    while (!pending.empty())
    {
        auto [name, distance] = pending.front();
        pending.pop();
        if (name == expected.name)
        {
            return distance;
        }
        for (const auto& base : classes_.at(name)->bases)
        {
            if (visited.insert(base).second)
            {
                pending.emplace(base, distance + 1);
            }
        }
    }
    return std::numeric_limits<std::size_t>::max();
}

bool semantic_analyzer::is_assignable(
    const value_type& actual, const value_type& expected) const
{
    return actual == expected ||
           class_distance(actual, expected) != std::numeric_limits<std::size_t>::max();
}

bool semantic_analyzer::same_overload_key(
    const function_decl& left, const function_decl& right)
{
    std::size_t index = 0;
    while (index < left.parameters.size() &&
           index < right.parameters.size() &&
           left.parameters[index].kind == parameter_kind::ordinary &&
           right.parameters[index].kind == parameter_kind::ordinary)
    {
        if (left.parameters[index].type != right.parameters[index].type)
        {
            return false;
        }
        ++index;
    }
    const bool left_done = index == left.parameters.size() ||
        left.parameters[index].kind != parameter_kind::ordinary;
    const bool right_done = index == right.parameters.size() ||
        right.parameters[index].kind != parameter_kind::ordinary;
    return left_done && right_done;
}

std::vector<const function_decl*> semantic_analyzer::collect_methods(
    const class_decl& type, std::string_view name) const
{
    std::vector<const function_decl*> result;
    for (const auto& base_name : type.bases)
    {
        for (const auto* method : collect_methods(*classes_.at(base_name), name))
        {
            if (std::find(result.begin(), result.end(), method) == result.end())
            {
                result.push_back(method);
            }
        }
    }
    // 若一个分支覆盖了共同祖先，保留该分支的方法；兄弟分支的覆盖仍可能冲突。
    const auto inherited = result;
    result.clear();
    for (const auto* candidate : inherited)
    {
        const bool dominated = std::any_of(inherited.begin(), inherited.end(),
            [&](const function_decl* other)
            {
                return candidate != other &&
                    same_overload_key(*candidate, *other) &&
                    class_distance(value_type(other->owner_class),
                                   value_type(candidate->owner_class)) !=
                        std::numeric_limits<std::size_t>::max();
            });
        if (!dominated)
        {
            result.push_back(candidate);
        }
    }
    for (const auto& method : type.methods)
    {
        if (method.name != name)
        {
            continue;
        }
        result.erase(std::remove_if(result.begin(), result.end(),
            [&](const function_decl* inherited)
            { return same_overload_key(method, *inherited); }), result.end());
        result.push_back(&method);
    }
    return result;
}

std::vector<semantic_analyzer::field_match> semantic_analyzer::collect_fields(
    const class_decl& type, std::string_view name) const
{
    std::vector<field_match> result;
    std::unordered_set<std::string> visited;
    const auto visit = [&](const auto& self, const class_decl& current) -> void
    {
        if (!visited.insert(current.name).second)
        {
            return;
        }
        for (const auto& base : current.bases)
        {
            self(self, *classes_.at(base));
        }
        for (const auto& field : current.fields)
        {
            if (field.name == name)
            {
                result.push_back({&field, &current});
            }
        }
    };
    visit(visit, type);
    return result;
}

const class_decl* semantic_analyzer::first_class_base(const class_decl& type) const
{
    for (const auto& base : type.bases)
    {
        const auto* found = classes_.at(base);
        if (!found->is_interface)
        {
            return found;
        }
    }
    return nullptr;
}

void semantic_analyzer::check_access(
    member_access access, const class_decl& owner,
    source_pos position, std::string_view member) const
{
    if (access == member_access::public_access ||
        (current_class_ != nullptr &&
         (current_class_ == &owner ||
          (access == member_access::protected_access &&
           class_distance(value_type(current_class_->name),
                          value_type(owner.name)) !=
               std::numeric_limits<std::size_t>::max()))))
    {
        return;
    }
    throw compile_error(position, "无权访问类成员：" + std::string(member));
}

} // namespace tx
