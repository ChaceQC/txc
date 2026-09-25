#include "frontend/sema/sema.hpp"

namespace tx
{

value_type semantic_analyzer::check_vector_call(
    expression& item, call_expression& call, const value_type& type)
{
    validate_type(type, item.position);
    const auto types = check_call_arguments(call);
    for (const auto& argument : call.arguments)
    {
        if (argument.kind != argument_kind::positional)
        {
            throw compile_error(argument.position, "vector 操作只接受普通位置实参");
        }
    }
    const auto& element = type.parameters.front();
    std::vector<value_type> expected;
    value_type result = value_type::void_type;
    if (call.container_type)
    {
        result = type;
        if (types.size() == 1)
        {
            expected = {value_type::array_type};
        }
        else if (!types.empty())
        {
            expected = {value_type::int_type, element};
        }
    }
    else if (call.name == "size" || call.name == "capacity")
    {
        result = value_type::int_type;
    }
    else if (call.name == "empty")
    {
        result = value_type::bool_type;
    }
    else if (call.name == "to_array")
    {
        result = value_type::array_type;
    }
    else if (call.name == "reserve" || call.name == "erase")
    {
        expected = {value_type::int_type};
    }
    else if (call.name == "push_back")
    {
        expected = {element};
    }
    else if (call.name == "resize" || call.name == "insert")
    {
        expected = {value_type::int_type, element};
    }
    else if (call.name != "clear" && call.name != "pop_back")
    {
        throw compile_error(item.position, "未知 vector 方法：" + call.name);
    }
    if (types != expected)
    {
        throw compile_error(item.position, "vector 操作参数数量或类型不匹配：" + call.name);
    }
    return result;
}

} // namespace tx
