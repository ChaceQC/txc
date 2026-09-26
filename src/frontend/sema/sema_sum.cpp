#include "frontend/sema/sema.hpp"

#include <initializer_list>
#include <string_view>

namespace tx
{
namespace
{

bool has_fields(const struct_decl& definition,
                std::initializer_list<std::pair<std::string_view, value_type>> fields)
{
    if (definition.fields.size() != fields.size())
    {
        return false;
    }
    std::size_t index = 0;
    for (const auto& [name, type] : fields)
    {
        if (definition.fields[index].name != name ||
            definition.fields[index].type != type)
        {
            return false;
        }
        ++index;
    }
    return true;
}

bool from_error_module(const struct_decl& definition)
{
    const auto& file = definition.position.file;
    return file.ends_with("/error.txh") || file.ends_with("\\error.txh");
}

bool is_error_info(const struct_decl& definition)
{
    return from_error_module(definition) &&
        definition.name.ends_with("error_info") &&
        has_fields(definition, {
            {"kind", value_type::str_type},
            {"code", value_type::str_type},
            {"message", value_type::str_type}});
}

} // namespace

value_type semantic_analyzer::check_sum_call(
    expression& item, call_expression& call, const value_type& type)
{
    validate_type(type, item.position);
    const auto types = check_call_arguments(call);
    for (const auto& argument : call.arguments)
    {
        if (argument.kind != argument_kind::positional)
        {
            throw compile_error(argument.position,
                "option/result 只接受普通位置实参");
        }
    }
    const auto& element = type.parameters.front();
    if (call.container_type)
    {
        if (type.is_option() &&
            (types.empty() || (types.size() == 1 && types[0] == element)))
        {
            return type;
        }
        if (type.is_result())
        {
            if (types.empty() && element == value_type::void_type)
            {
                return type;
            }
            if (types.size() == 1 && types[0] == element &&
                element != value_type::void_type)
            {
                return type;
            }
            if (types.size() == 1)
            {
                const auto found = structs_.find(types[0].name);
                if (found != structs_.end() &&
                    from_error_module(*found->second) &&
                    found->second->name.ends_with("_result") &&
                    found->second->fields.size() == 3 &&
                    has_fields(*found->second, {
                        {"ok", value_type::bool_type}, {"value", element},
                        {"error", found->second->fields[2].type}}))
                {
                    const auto error = structs_.find(found->second->fields[2].type.name);
                    if (error != structs_.end() &&
                        is_error_info(*error->second))
                    {
                        return type;
                    }
                }
            }
            if (types.size() == 2)
            {
                const auto* flag = std::get_if<boolean_literal>(
                    &call.arguments[0].value->data);
                const auto found = structs_.find(types[1].name);
                if (flag && !flag->value && found != structs_.end() &&
                    is_error_info(*found->second))
                {
                    return type;
                }
            }
        }
        throw compile_error(item.position,
            type.container_name() + " 构造参数类型不匹配");
    }
    if (types.empty())
    {
        if (type.is_option())
        {
            if (call.name == "is_some" || call.name == "is_none")
            {
                return value_type::bool_type;
            }
            if (call.name == "value")
            {
                return element;
            }
        }
        else
        {
            if (call.name == "is_ok" || call.name == "is_err")
            {
                return value_type::bool_type;
            }
            if (call.name == "value")
            {
                return element;
            }
            if (call.name == "error")
            {
                value_type error_type = value_type::unknown_type;
                for (const auto& [name, definition] : structs_)
                {
                    if (!is_error_info(*definition))
                    {
                        continue;
                    }
                    if (error_type != value_type::unknown_type)
                    {
                        throw compile_error(item.position,
                            "error_info 类型存在歧义");
                    }
                    error_type = value_type(name);
                }
                if (error_type == value_type::unknown_type)
                {
                    throw compile_error(item.position,
                        "使用 result.error() 前需要导入 error.txh");
                }
                return error_type;
            }
        }
    }
    if (type.is_option() && call.name == "value_or" &&
        types.size() == 1 && types[0] == element)
    {
        return element;
    }
    throw compile_error(item.position,
        type.container_name() + " 方法参数数量或类型不匹配：" + call.name);
}

} // namespace tx
