#include "frontend/sema/sema.hpp"

namespace tx
{
namespace
{

struct container_signature
{
    value_type result;
    std::vector<value_type> parameters;
};

container_signature method_signature(const value_type& type,
                                     const std::string& name, source_pos position)
{
    const auto& element = type.parameters.front();
    const auto kind = type.container_name();
    if (name == "size")
    {
        return {value_type::int_type, {}};
    }
    if (name == "empty")
    {
        return {value_type::bool_type, {}};
    }
    if (name == "clear")
    {
        return {value_type::void_type, {}};
    }
    if ((kind == "map" || kind == "set") &&
        (name == "contains" || name == "remove" || (kind == "set" && name == "insert")))
    {
        return {value_type::bool_type, {element}};
    }
    if (kind == "map" && name == "get")
    {
        return {type.parameters[1], {element, type.parameters[1]}};
    }
    if (kind == "map" && (name == "keys" || name == "values"))
    {
        return {value_type::vector_of(type.parameters[name == "keys" ? 0 : 1]), {}};
    }
    if (kind != "map" && name == "to_vector")
    {
        return {value_type::vector_of(element), {}};
    }
    if (kind == "heap" || kind == "queue")
    {
        if (name == "push")
        {
            return {value_type::void_type, {element}};
        }
        if (name == "pop")
        {
            return {value_type::void_type, {}};
        }
        if ((kind == "heap" && name == "top") ||
            (kind == "queue" && (name == "front" || name == "back")))
        {
            return {element, {}};
        }
    }
    throw compile_error(position, "未知 " + kind + " 方法：" + name);
}

} // namespace

value_type semantic_analyzer::check_container_call(
    expression& item, call_expression& call, const value_type& type)
{
    validate_type(type, item.position);
    const auto actual = check_call_arguments(call);
    for (const auto& argument : call.arguments)
    {
        if (argument.kind != argument_kind::positional)
        {
            throw compile_error(argument.position, "类型化容器操作只接受普通位置实参");
        }
    }
    container_signature signature{type, {}};
    if (call.container_type)
    {
        if (type.container_name() == "heap" && !actual.empty())
        {
            signature.parameters = {value_type::bool_type};
        }
    }
    else
    {
        signature = method_signature(type, call.name, item.position);
    }
    if (actual != signature.parameters)
    {
        throw compile_error(item.position, type.container_name() +
            " 操作参数数量或类型不匹配：" + call.name);
    }
    return signature.result;
}

} // namespace tx
