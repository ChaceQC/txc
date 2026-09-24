#include "frontend/sema/sema.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

namespace tx
{
namespace
{

bool is_numeric(const value_type& type)
{
    return type == value_type::int_type || type == value_type::float_type;
}

bool is_equality_type(const value_type& type)
{
    return is_numeric(type) || type == value_type::bool_type ||
           type == value_type::str_type || type == value_type::none_type;
}

bool is_printable(const value_type& type)
{
    return is_numeric(type) || type == value_type::bool_type ||
           type == value_type::str_type || type == value_type::none_type ||
           type == value_type::any_type;
}

bool is_min_int_magnitude(std::string_view digits)
{
    const auto first = digits.find_first_not_of('0');
    return first != std::string_view::npos &&
           digits.substr(first) == "9223372036854775808";
}

} // namespace

value_type semantic_analyzer::check_literal(expression& item)
{
    if (auto* literal = std::get_if<integer_literal>(&item.data))
    {
        std::int64_t value = 0;
        const auto* end = literal->digits.data() + literal->digits.size();
        const auto [parsed, error] =
            std::from_chars(literal->digits.data(), end, value);
        if (error != std::errc{} || parsed != end)
        {
            throw compile_error(item.position, "整数常量超出 int 范围");
        }
        return value_type::int_type;
    }
    if (auto* literal = std::get_if<floating_literal>(&item.data))
    {
        double value = 0.0;
        const auto* end = literal->digits.data() + literal->digits.size();
        const auto [parsed, error] =
            std::from_chars(literal->digits.data(), end, value);
        if (error != std::errc{} || parsed != end || !std::isfinite(value))
        {
            throw compile_error(item.position, "浮点常量超出 float 范围");
        }
        return value_type::float_type;
    }
    if (std::holds_alternative<string_literal>(item.data))
    {
        return value_type::str_type;
    }
    if (std::holds_alternative<boolean_literal>(item.data))
    {
        return value_type::bool_type;
    }
    if (std::holds_alternative<none_literal>(item.data))
    {
        return value_type::none_type;
    }
    auto& literal = std::get<array_literal>(item.data);
    for (auto& element : literal.elements)
    {
        if (check_expression(*element) == value_type::void_type)
        {
            throw compile_error(element->position, "数组不能存放无返回值的调用");
        }
    }
    return value_type::array_type;
}

value_type semantic_analyzer::check_unary(expression& item,
                                          unary_operation& operation)
{
    if (operation.operation == token_kind::minus)
    {
        const auto* literal = std::get_if<integer_literal>(&operation.operand->data);
        if (literal != nullptr && is_min_int_magnitude(literal->digits))
        {
            // 最小 int 的绝对值超出正数范围，只允许紧随一元负号使用。
            operation.is_min_int_literal = true;
            operation.operand->type = value_type::int_type;
            return value_type::int_type;
        }
    }
    const auto actual = check_expression(*operation.operand);
    if (operation.operation == token_kind::minus && is_numeric(actual))
    {
        return actual;
    }
    if (operation.operation == token_kind::bang &&
        actual == value_type::bool_type)
    {
        return value_type::bool_type;
    }
    throw compile_error(item.position, "一元运算符与操作数类型不匹配");
}

value_type semantic_analyzer::check_binary(expression& item,
                                           binary_operation& operation)
{
    const auto left = check_expression(*operation.left);
    const auto right = check_expression(*operation.right);
    switch (operation.operation)
    {
    case token_kind::plus:
        if (left == value_type::str_type && right == value_type::str_type)
        {
            return value_type::str_type;
        }
        [[fallthrough]];
    case token_kind::minus:
    case token_kind::star:
    case token_kind::slash:
        if (left == right && is_numeric(left))
        {
            return left;
        }
        break;
    case token_kind::less:
    case token_kind::less_equal:
    case token_kind::greater:
    case token_kind::greater_equal:
        if (left == right && is_numeric(left))
        {
            return value_type::bool_type;
        }
        break;
    case token_kind::equal_equal:
    case token_kind::bang_equal:
        if (left == right && is_equality_type(left))
        {
            return value_type::bool_type;
        }
        break;
    case token_kind::and_and:
    case token_kind::or_or:
        if (left == value_type::bool_type && right == value_type::bool_type)
        {
            return value_type::bool_type;
        }
        break;
    default:
        break;
    }
    throw compile_error(item.position, "二元运算符与操作数类型不匹配：" +
                                       std::string(type_name(left)) + " 和 " +
                                       std::string(type_name(right)));
}

value_type semantic_analyzer::check_builtin(expression& item, call_expression& call)
{
    if (call.name == "input")
    {
        if (call.arguments.size() > 1)
        {
            throw compile_error(item.position, "input 最多接收一个提示字符串");
        }
        if (!call.arguments.empty())
        {
            require_type(check_expression(*call.arguments.front()),
                         value_type::str_type,
                         call.arguments.front()->position, "input 提示");
        }
        return value_type::str_type;
    }
    if (call.arguments.size() != 1)
    {
        throw compile_error(item.position, call.name + " 需要一个参数");
    }
    auto& argument = *call.arguments.front();
    const auto actual = check_expression(argument);
    if (call.name == "print")
    {
        if (!is_printable(actual))
        {
            throw compile_error(argument.position, "print 不支持此类型：" +
                                                    std::string(type_name(actual)));
        }
        return value_type::void_type;
    }
    if (call.name == "len")
    {
        if (actual != value_type::array_type &&
            actual != value_type::str_type &&
            actual != value_type::any_type)
        {
            throw compile_error(argument.position, "len 需要数组或字符串");
        }
        return value_type::int_type;
    }
    if (call.name == "to_float")
    {
        require_type(actual, value_type::int_type, argument.position, "to_float 参数");
        return value_type::float_type;
    }
    if (actual != value_type::any_type && actual != value_type::none_type)
    {
        throw compile_error(argument.position, "is_none 只接受数组元素或 none");
    }
    return value_type::bool_type;
}

value_type semantic_analyzer::check_constructor(expression& item,
                                                call_expression& call)
{
    const auto& definition = *structs_.at(call.name);
    if (call.arguments.size() != definition.fields.size())
    {
        throw compile_error(item.position, "结构体构造参数数量不匹配：" + call.name);
    }
    for (std::size_t index = 0; index < call.arguments.size(); ++index)
    {
        require_type(check_expression(*call.arguments[index]),
                     definition.fields[index].type,
                     call.arguments[index]->position, "结构体字段初值");
    }
    call.is_constructor = true;
    return value_type(call.name);
}

value_type semantic_analyzer::check_call(expression& item, call_expression& call)
{
    const auto& display_name = call.source_name.empty()
        ? call.name : call.source_name;
    if (call.name == "print" || call.name == "len" ||
        call.name == "to_float" || call.name == "input" ||
        call.name == "is_none")
    {
        return check_builtin(item, call);
    }
    if (structs_.contains(call.name))
    {
        return check_constructor(item, call);
    }
    const auto found = functions_.find(call.name);
    if (found == functions_.end())
    {
        throw compile_error(item.position, "未定义函数：" + display_name);
    }
    bool valid_arity = false;
    for (const auto& signature : found->second)
    {
        if (call.arguments.size() == signature.parameters.size())
        {
            valid_arity = true;
            break;
        }
    }
    if (!valid_arity)
    {
        throw compile_error(item.position, "函数参数数量不匹配：" + display_name);
    }
    std::vector<value_type> actual_types;
    actual_types.reserve(call.arguments.size());
    for (auto& argument : call.arguments)
    {
        actual_types.push_back(check_expression(*argument));
    }
    for (const auto& signature : found->second)
    {
        if (signature.parameters == actual_types)
        {
            return signature.result;
        }
    }
    std::string actual = display_name + "(";
    for (std::size_t index = 0; index < actual_types.size(); ++index)
    {
        if (index != 0)
        {
            actual += ", ";
        }
        actual += type_name(actual_types[index]);
    }
    throw compile_error(item.position, "没有匹配的函数重载：" + actual + ")");
}

value_type semantic_analyzer::check_index(expression&,
                                          index_expression& access)
{
    const auto object_type = check_expression(*access.object);
    if (object_type != value_type::array_type &&
        object_type != value_type::any_type)
    {
        throw compile_error(access.object->position, "索引对象需要数组");
    }
    require_type(check_expression(*access.index), value_type::int_type,
                 access.index->position, "数组索引");
    return value_type::any_type;
}

value_type semantic_analyzer::check_member(expression& item,
                                           member_expression& access)
{
    const auto object_type = check_expression(*access.object);
    if (object_type == value_type::any_type)
    {
        return value_type::any_type;
    }
    const auto found = structs_.find(object_type.name);
    if (found == structs_.end())
    {
        throw compile_error(item.position, "字段访问需要结构体类型");
    }
    for (const auto& field : found->second->fields)
    {
        if (field.name == access.field)
        {
            return field.type;
        }
    }
    throw compile_error(item.position, "未知字段：" + access.field);
}

value_type semantic_analyzer::check_cast(expression& item, cast_expression& cast)
{
    const auto numeric_target = is_numeric(cast.target);
    if (!numeric_target && cast.target != value_type::str_type)
    {
        throw compile_error(item.position, "as 目前只支持 int、float 和 str 转换");
    }
    const auto actual = check_expression(*cast.value);
    const auto numeric_source = is_numeric(actual) ||
                                actual == value_type::str_type ||
                                actual == value_type::any_type;
    const auto string_source = numeric_source || actual == value_type::bool_type;
    if ((numeric_target && !numeric_source) ||
        (!numeric_target && !string_source))
    {
        throw compile_error(item.position, "as 的源类型与目标类型不兼容");
    }
    return cast.target;
}

value_type semantic_analyzer::check_expression(expression& item)
{
    if (std::holds_alternative<integer_literal>(item.data) ||
        std::holds_alternative<floating_literal>(item.data) ||
        std::holds_alternative<string_literal>(item.data) ||
        std::holds_alternative<boolean_literal>(item.data) ||
        std::holds_alternative<none_literal>(item.data) ||
        std::holds_alternative<array_literal>(item.data))
    {
        item.type = check_literal(item);
    }
    else if (auto* name = std::get_if<name_reference>(&item.data))
    {
        const auto* symbol = find_symbol(name->name);
        if (symbol == nullptr)
        {
            throw compile_error(item.position, "未定义变量：" + name->name);
        }
        item.type = symbol->type;
    }
    else if (auto* access = std::get_if<index_expression>(&item.data))
    {
        item.type = check_index(item, *access);
    }
    else if (auto* access = std::get_if<member_expression>(&item.data))
    {
        item.type = check_member(item, *access);
    }
    else if (auto* cast = std::get_if<cast_expression>(&item.data))
    {
        item.type = check_cast(item, *cast);
    }
    else if (auto* operation = std::get_if<unary_operation>(&item.data))
    {
        item.type = check_unary(item, *operation);
    }
    else if (auto* operation = std::get_if<binary_operation>(&item.data))
    {
        item.type = check_binary(item, *operation);
    }
    else if (auto* call = std::get_if<call_expression>(&item.data))
    {
        item.type = check_call(item, *call);
    }
    return item.type;
}

} // namespace tx
