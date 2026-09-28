#include "frontend/sema/sema.hpp"

#include <algorithm>
#include <limits>
#include <unordered_set>

namespace tx
{

void semantic_analyzer::register_classes(program& source)
{
    std::unordered_map<std::string, int> state;
    for (auto& definition : source.classes)
    {
        register_class(definition, source, state);
    }
}

void semantic_analyzer::register_class(
    class_decl& definition, program& source,
    std::unordered_map<std::string, int>& state)
{
    if (state[definition.name] == 2)
    {
        return;
    }
    if (state[definition.name] == 1)
    {
        throw compile_error(definition.position,
                            "类或接口继承形成循环：" + definition.source_name);
    }
    state[definition.name] = 1;
    std::unordered_set<std::string> direct_bases;
    std::unordered_set<std::size_t> conflicts;
    for (const auto& base_name : definition.bases)
    {
        const auto found = classes_.find(base_name);
        if (found == classes_.end() || !direct_bases.insert(base_name).second)
        {
            throw compile_error(definition.position,
                                "未知或重复的继承类型：" + base_name);
        }
        auto& base = *found->second;
        if (definition.is_interface && !base.is_interface)
        {
            throw compile_error(definition.position,
                                "interface 只能继承 interface：" + base_name);
        }
        register_class(base, source, state);
        for (const auto slot : base.known_virtual_slots)
        {
            if (std::find(definition.known_virtual_slots.begin(),
                          definition.known_virtual_slots.end(), slot) ==
                definition.known_virtual_slots.end())
            {
                definition.known_virtual_slots.push_back(slot);
            }
            if (definition.virtual_targets.size() <= slot)
            {
                definition.virtual_targets.resize(slot + 1);
            }
            const auto& incoming = base.virtual_targets[slot];
            auto& selected = definition.virtual_targets[slot];
            if (conflicts.contains(slot) || incoming.symbol.empty())
            {
                continue;
            }
            if (selected.symbol.empty() ||
                class_distance(value_type(incoming.owner_class),
                               value_type(selected.owner_class)) !=
                    std::numeric_limits<std::size_t>::max())
            {
                selected = incoming;
            }
            else if (class_distance(value_type(selected.owner_class),
                                    value_type(incoming.owner_class)) ==
                     std::numeric_limits<std::size_t>::max())
            {
                selected = {};
                conflicts.insert(slot);
            }
        }
    }
    std::unordered_set<std::string> own_fields;
    for (const auto& field : definition.fields)
    {
        if (is_lock_guard_type(field.type))
        {
            throw compile_error(field.position,
                "锁卫士不能作为类字段");
        }
        if (field.type == value_type::secret_bytes_type)
        {
            throw compile_error(field.position,
                "secret_bytes 不能作为类字段");
        }
        if (field.type.is_function())
        {
            throw compile_error(field.position, "类字段暂不支持 fn 类型");
        }
        validate_type(field.type, field.position);
        if (!own_fields.insert(field.name).second)
        {
            throw compile_error(field.position, "重复类字段：" + field.name);
        }
        for (const auto& base : definition.bases)
        {
            if (!collect_fields(*classes_.at(base), field.name).empty())
            {
                throw compile_error(field.position,
                                    "不能重声明继承字段：" + field.name);
            }
        }
    }
    register_class_methods(definition, source);
    if (!definition.is_abstract && !definition.is_interface)
    {
        for (const auto slot : definition.known_virtual_slots)
        {
            if (definition.virtual_targets[slot].symbol.empty())
            {
                throw compile_error(definition.position,
                                    "具体类缺少抽象方法实现或虚方法覆盖存在歧义：" +
                                    definition.source_name);
            }
        }
    }
    state[definition.name] = 2;
}

void semantic_analyzer::register_class_methods(
    class_decl& definition, program& source)
{
    for (std::size_t index = 0; index < definition.methods.size(); ++index)
    {
        auto& method = definition.methods[index];
        std::size_t overload = 0;
        for (std::size_t prior = 0; prior < index; ++prior)
        {
            const auto& previous = definition.methods[prior];
            if (previous.name != method.name)
            {
                continue;
            }
            if (same_overload_key(previous, method))
            {
                throw compile_error(method.position,
                                    "重复的方法重载签名：" + method.name);
            }
            ++overload;
        }
        method.overload_index = overload;
        for (const auto& field : definition.fields)
        {
            if (field.name == method.name)
            {
                throw compile_error(method.position,
                                    "字段与方法重名：" + method.name);
            }
        }
        register_class_method(definition, method, source);
    }
}

void semantic_analyzer::register_class_method(
    class_decl& definition, function_decl& method, program& source)
{
    if (method.return_type != value_type::void_type)
    {
        validate_type(method.return_type, method.position);
    }
    std::unordered_set<std::string> parameter_names;
    for (const auto& parameter : method.parameters)
    {
        validate_type(parameter.type, parameter.position);
        if (parameter.name == "self" || parameter.name == "super" ||
            !parameter_names.insert(parameter.name).second)
        {
            throw compile_error(parameter.position,
                                "重复或保留的方法参数名：" + parameter.name);
        }
    }
    if (method.operator_kind)
    {
        validate_operator_method(method);
    }
    if (method.name == "init" || method.name == "deinit")
    {
        if (definition.is_interface ||
            method.return_type != value_type::void_type ||
            method.is_virtual || method.is_override || method.is_abstract ||
            (method.name == "init" &&
             method.access != member_access::public_access) ||
            (method.name == "deinit" && !method.parameters.empty()))
        {
            throw compile_error(method.position,
                                "init/deinit 的声明或修饰符不合法");
        }
        return;
    }
    if (method.is_abstract && !definition.is_abstract &&
        !definition.is_interface)
    {
        throw compile_error(method.position,
                            "abstract def 只能出现在抽象类或接口中");
    }
    std::vector<const function_decl*> inherited;
    for (const auto& base : definition.bases)
    {
        for (const auto* candidate : collect_methods(*classes_.at(base), method.name))
        {
            if (same_overload_key(method, *candidate) &&
                std::find(inherited.begin(), inherited.end(), candidate) ==
                    inherited.end())
            {
                inherited.push_back(candidate);
            }
        }
    }
    if (inherited.empty())
    {
        if (method.is_override)
        {
            throw compile_error(method.position,
                                "override 没有对应的父类方法：" + method.name);
        }
        if (method.is_virtual || method.is_abstract)
        {
            const auto slot = source.virtual_slot_count++;
            method.virtual_slots.push_back(slot);
            definition.known_virtual_slots.push_back(slot);
            if (definition.virtual_targets.size() <= slot)
            {
                definition.virtual_targets.resize(slot + 1);
            }
            if (!method.is_abstract)
            {
                definition.virtual_targets[slot] = {
                    class_method_symbol(definition.name, method.name),
                    method.overload_index, definition.name};
            }
        }
        return;
    }
    if (!method.is_override)
    {
        throw compile_error(method.position,
                            "覆盖继承签名需要 override：" + method.name);
    }
    for (const auto* parent : inherited)
    {
        if (parent->virtual_slots.empty() ||
            method.access < parent->access ||
            method.return_type != parent->return_type ||
            method.parameters.size() != parent->parameters.size())
        {
            throw compile_error(method.position,
                                "override 的目标、权限或签名不匹配：" + method.name);
        }
        for (std::size_t i = 0; i < method.parameters.size(); ++i)
        {
            const auto& left = method.parameters[i];
            const auto& right = parent->parameters[i];
            if (left.name != right.name || left.type != right.type ||
                left.kind != right.kind)
            {
                throw compile_error(left.position,
                                    "override 参数不匹配：" + method.name);
            }
        }
        for (const auto slot : parent->virtual_slots)
        {
            if (std::find(method.virtual_slots.begin(), method.virtual_slots.end(),
                          slot) == method.virtual_slots.end())
            {
                method.virtual_slots.push_back(slot);
            }
        }
    }
    for (const auto slot : method.virtual_slots)
    {
        definition.virtual_targets[slot] = {
            class_method_symbol(definition.name, method.name),
            method.overload_index, definition.name};
    }
}

} // namespace tx
