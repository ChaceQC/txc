#include "frontend/resolver/module_resolver.hpp"

#include <stdexcept>
#include <unordered_set>

namespace tx
{
namespace
{

bool is_builtin_call(const std::string& name)
{
    return name == "print" || name == "len" || name == "input" ||
           name == "input_or_none" ||
           name == "to_float" || name == "is_none" ||
           name == "self" || name == "super";
}

bool is_builtin_type(const std::string& name)
{
    return name == "int" || name == "float" || name == "str" ||
           name == "bool" || name == "array" || name == "dict" || name == "any" ||
           name == "none" || name == "void" || name == "unknown";
}

} // namespace

std::string module_resolver::module_of(const source_pos& position) const
{
    const auto found = source_->file_modules.find(position.file);
    if (found == source_->file_modules.end())
    {
        throw std::logic_error("模块来源丢失：" + position.file);
    }
    return found->second;
}

void module_resolver::build_index(const program& source)
{
    source_ = &source;
    scopes_.clear();
    prefixes_.clear();
    exports_.clear();
    for (std::size_t index = 0; index < source.modules.size(); ++index)
    {
        const auto& module = source.modules[index];
        scopes_.emplace(module.key, &module);
        prefixes_.emplace(module.key, "m" + std::to_string(index) + "_");
    }
    for (const auto& definition : source.structs)
    {
        const auto key = module_of(definition.position);
        if (is_builtin_type(definition.name) || is_builtin_call(definition.name))
        {
            throw compile_error(definition.position,
                                "保留的结构体名：" + definition.name);
        }
        const auto internal = prefixes_.at(key) + definition.name;
        if (!exports_[key].emplace(
                definition.name,
                export_entry{internal, true, definition.position}).second)
        {
            throw compile_error(definition.position,
                                "模块内重复声明：" + definition.name);
        }
    }
    for (const auto& definition : source.classes)
    {
        const auto key = module_of(definition.position);
        if (is_builtin_type(definition.name) || is_builtin_call(definition.name))
        {
            throw compile_error(definition.position, "保留的类名：" + definition.name);
        }
        const auto internal = prefixes_.at(key) + definition.name;
        if (!exports_[key].emplace(
                definition.name,
                export_entry{internal, true, definition.position}).second)
        {
            throw compile_error(definition.position,
                                "模块内重复声明：" + definition.name);
        }
    }
    for (const auto& function : source.functions)
    {
        const auto key = module_of(function.position);
        if (is_builtin_call(function.name) || is_builtin_type(function.name))
        {
            throw compile_error(function.position, "保留的函数名：" + function.name);
        }
        const auto internal = key == source.root_module && function.name == "main"
            ? "main" : prefixes_.at(key) + function.name;
        const auto [found, inserted] = exports_[key].emplace(
            function.name, export_entry{internal, false, function.position});
        if (!inserted && found->second.is_type)
        {
            throw compile_error(function.position,
                                "模块内重复声明：" + function.name);
        }
    }
    for (const auto& module : source.modules)
    {
        for (const auto& dependency : module.imports)
        {
            if (!dependency.alias.empty() &&
                exports_[module.key].contains(dependency.alias))
            {
                throw compile_error(dependency.position,
                                    "模块别名与本地声明重名：" + dependency.alias);
            }
        }
    }
}

const module_resolver::export_entry* module_resolver::find_export(
    const std::string& module_key, const std::string& name) const
{
    const auto module = exports_.find(module_key);
    if (module == exports_.end())
    {
        return nullptr;
    }
    const auto symbol = module->second.find(name);
    return symbol == module->second.end() ? nullptr : &symbol->second;
}

const module_resolver::export_entry* module_resolver::find_symbol(
    const std::string& module_key, const std::string& name,
    source_pos position, bool type_only) const
{
    const auto& scope = *scopes_.at(module_key);
    if (const auto dot = name.find('.'); dot != std::string::npos)
    {
        const auto alias = name.substr(0, dot);
        const auto member = name.substr(dot + 1);
        for (const auto& dependency : scope.imports)
        {
            if (dependency.alias == alias && !alias.empty())
            {
                const auto* result = find_export(dependency.target, member);
                if (result == nullptr || (type_only && !result->is_type))
                {
                    throw compile_error(position, "模块中不存在该名称：" + name);
                }
                return result;
            }
        }
        throw compile_error(position, "未知模块别名：" + alias);
    }
    if (const auto* own = find_export(module_key, name);
        own != nullptr && (!type_only || own->is_type))
    {
        return own;
    }
    const export_entry* selected = nullptr;
    std::unordered_set<std::string> visited;
    for (const auto& dependency : scope.imports)
    {
        if (!dependency.alias.empty() || !visited.insert(dependency.target).second)
        {
            continue;
        }
        const auto* candidate = find_export(dependency.target, name);
        if (candidate == nullptr || (type_only && !candidate->is_type))
        {
            continue;
        }
        if (selected != nullptr)
        {
            throw compile_error(position,
                                "导入名称存在歧义，请使用 as 别名：" + name);
        }
        selected = candidate;
    }
    return selected;
}

value_type module_resolver::resolve_type(
    const std::string& module_key, const value_type& type,
    source_pos position) const
{
    if (type.is_vector())
    {
        return value_type::vector_of(resolve_type(
            module_key, type.parameters.front(), position));
    }
    if (is_builtin_type(type.name))
    {
        return type;
    }
    const auto* symbol = find_symbol(module_key, type.name, position, true);
    if (symbol == nullptr)
    {
        throw compile_error(position, "未知类型：" + type.name);
    }
    return value_type(symbol->internal_name);
}

std::string module_resolver::resolve_call_name(
    const std::string& module_key, const std::string& name,
    source_pos position) const
{
    if (name.find('.') == std::string::npos && is_builtin_call(name))
    {
        return name;
    }
    const auto* symbol = find_symbol(module_key, name, position, false);
    return symbol == nullptr ? name : symbol->internal_name;
}

void module_resolver::resolve(program& source)
{
    build_index(source);
    for (auto& definition : source.structs)
    {
        const auto key = module_of(definition.position);
        for (auto& field : definition.fields)
        {
            field.type = resolve_type(key, field.type, field.position);
        }
        for (auto& method : definition.methods)
        {
            for (auto& parameter : method.parameters)
            {
                check_local_name(key, parameter.name, parameter.position);
                parameter.type = resolve_type(key, parameter.type,
                                              parameter.position);
            }
            method.return_type = resolve_type(
                key, method.return_type, method.position);
            resolve_statements(method.body, key);
            method.owner_class = exports_.at(key).at(definition.name).internal_name;
        }
        definition.name = exports_.at(key).at(definition.name).internal_name;
    }
    for (auto& definition : source.classes)
    {
        const auto key = module_of(definition.position);
        for (auto& base : definition.bases)
        {
            base = resolve_type(key, value_type(base), definition.position).name;
        }
        for (auto& field : definition.fields)
        {
            field.type = resolve_type(key, field.type, field.position);
        }
        for (auto& method : definition.methods)
        {
            for (auto& parameter : method.parameters)
            {
                check_local_name(key, parameter.name, parameter.position);
                parameter.type = resolve_type(key, parameter.type, parameter.position);
            }
            method.return_type = resolve_type(
                key, method.return_type, method.position);
            resolve_statements(method.body, key);
            method.owner_class = exports_.at(key).at(definition.name).internal_name;
        }
        definition.name = exports_.at(key).at(definition.name).internal_name;
    }
    for (auto& function : source.functions)
    {
        const auto key = module_of(function.position);
        for (auto& parameter : function.parameters)
        {
            check_local_name(key, parameter.name, parameter.position);
            parameter.type = resolve_type(key, parameter.type, parameter.position);
        }
        function.return_type = resolve_type(
            key, function.return_type, function.position);
        resolve_statements(function.body, key);
        function.source_name = function.name;
        function.name = exports_.at(key).at(function.name).internal_name;
    }
}

} // namespace tx
