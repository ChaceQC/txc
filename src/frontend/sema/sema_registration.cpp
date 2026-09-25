#include "frontend/sema/sema.hpp"

#include <unordered_set>
#include <utility>

namespace tx
{
namespace
{

bool is_builtin_name(const std::string& name)
{
    return name == "print" || name == "len" || name == "array" ||
           name == "dict" || name == "to_float" || name == "is_none" ||
           name == "input" || name == "input_or_none" ||
           name == "any" || name == "void" || name == "unknown" ||
           name == "self" || name == "super";
}

} // namespace

void semantic_analyzer::validate_type(const value_type& type, source_pos position) const
{
    if (type == value_type::int_type || type == value_type::bool_type ||
        type == value_type::float_type || type == value_type::str_type ||
        type == value_type::array_type || type == value_type::dict_type ||
        type == value_type::any_type ||
        structs_.contains(type.name) || classes_.contains(type.name))
    {
        return;
    }
    throw compile_error(position, "未知类型：" + type.name);
}

void semantic_analyzer::register_structs(program& source)
{
    structs_.clear();
    for (const auto& definition : source.structs)
    {
        if (structs_.contains(definition.name) || classes_.contains(definition.name) ||
            is_builtin_name(definition.name))
        {
            throw compile_error(definition.position, "重复或保留的结构体名：" +
                                                      definition.name);
        }
        std::unordered_set<std::string> fields;
        for (const auto& field : definition.fields)
        {
            validate_type(field.type, field.position);
            if (!fields.insert(field.name).second)
            {
                throw compile_error(field.position, "重复字段：" + field.name);
            }
        }
        // 只允许字段引用已经完成声明的结构体，避免生成递归值类型。
        structs_.emplace(definition.name, &definition);
    }
    for (auto& definition : source.structs)
    {
        for (std::size_t index = 0; index < definition.methods.size(); ++index)
        {
            auto& method = definition.methods[index];
            if (method.return_type != value_type::void_type)
            {
                validate_type(method.return_type, method.position);
            }
            std::unordered_set<std::string> names;
            for (const auto& parameter : method.parameters)
            {
                validate_type(parameter.type, parameter.position);
                if (parameter.name == "self" || parameter.name == "super" ||
                    !names.insert(parameter.name).second)
                {
                    throw compile_error(parameter.position,
                                        "重复或保留的运算符参数名：" +
                                        parameter.name);
                }
            }
            validate_operator_method(method);
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
                                        "重复的运算符重载签名：" +
                                        method.source_name);
                }
                ++overload;
            }
            method.overload_index = overload;
        }
    }
}

void semantic_analyzer::register_functions(const program& source, bool require_main)
{
    functions_.clear();
    for (const auto& function : source.functions)
    {
        const auto& display_name = function.source_name.empty()
            ? function.name : function.source_name;
        if (is_builtin_name(function.name) || structs_.contains(function.name) ||
            classes_.contains(function.name))
        {
            throw compile_error(function.position, "重复或保留的函数名：" + display_name);
        }
        if (function.external && function.name == "main")
        {
            throw compile_error(function.position, ".txh 不能声明 main");
        }
        if (function.return_type != value_type::void_type)
        {
            validate_type(function.return_type, function.position);
        }
        function_signature signature{{}, function.return_type};
        for (const auto& parameter : function.parameters)
        {
            validate_type(parameter.type, parameter.position);
            signature.parameters.push_back(parameter);
        }
        auto& overloads = functions_[function.name];
        if (function.name == "main" && !overloads.empty())
        {
            throw compile_error(function.position, "main 不能重载");
        }
        for (const auto& existing : overloads)
        {
            bool same_fixed_types = true;
            std::size_t left = 0;
            std::size_t right = 0;
            while (left < existing.parameters.size() &&
                   existing.parameters[left].kind == parameter_kind::ordinary &&
                   right < signature.parameters.size() &&
                   signature.parameters[right].kind == parameter_kind::ordinary)
            {
                if (existing.parameters[left].type != signature.parameters[right].type)
                {
                    same_fixed_types = false;
                    break;
                }
                ++left;
                ++right;
            }
            if (same_fixed_types &&
                (left == existing.parameters.size() ||
                 existing.parameters[left].kind != parameter_kind::ordinary) &&
                (right == signature.parameters.size() ||
                 signature.parameters[right].kind != parameter_kind::ordinary))
            {
                throw compile_error(function.position,
                                    "重复的函数重载签名：" + display_name);
            }
        }
        overloads.push_back(std::move(signature));
    }
    if (!require_main)
    {
        return;
    }
    const auto main = functions_.find("main");
    if (main == functions_.end())
    {
        throw compile_error({1, 1, {}}, "缺少 main 函数");
    }
    const auto& entry = main->second.front();
    if (!entry.parameters.empty() || entry.result != value_type::int_type)
    {
        throw compile_error({1, 1, {}}, "main 必须声明为 def main() -> int");
    }
}

} // namespace tx
