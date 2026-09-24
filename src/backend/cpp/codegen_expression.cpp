#include "backend/cpp/codegen.hpp"

#include <string>

namespace tx
{

std::string code_generator::cpp_type(const value_type& type)
{
    if (type == value_type::void_type)
    {
        return "void";
    }
    if (type == value_type::int_type)
    {
        return "tx_int";
    }
    if (type == value_type::float_type)
    {
        return "double";
    }
    if (type == value_type::bool_type)
    {
        return "bool";
    }
    if (type == value_type::str_type)
    {
        return "std::string";
    }
    if (type == value_type::array_type)
    {
        return "tx_array";
    }
    if (type == value_type::any_type || type == value_type::none_type)
    {
        return "std::any";
    }
    return cpp_struct(type.name);
}

std::string code_generator::cpp_function(const std::string& name)
{
    return "tx_fn_" + name;
}

std::string code_generator::cpp_variable(const std::string& name)
{
    return "tx_var_" + name;
}

std::string code_generator::cpp_field(const std::string& name)
{
    return "tx_field_" + name;
}

std::string code_generator::cpp_struct(const std::string& name)
{
    return "tx_struct_" + name;
}

std::string code_generator::function_header(const function_decl& function)
{
    std::string result = cpp_type(function.return_type) + " " +
                         cpp_function(function.name) + "(";
    for (std::size_t index = 0; index < function.parameters.size(); ++index)
    {
        if (index != 0)
        {
            result += ", ";
        }
        result += cpp_type(function.parameters[index].type) + " " +
                  cpp_variable(function.parameters[index].name);
    }
    return result + ")";
}

std::string code_generator::emit_call(const call_expression& call)
{
    std::string function_name = cpp_function(call.name);
    if (call.name == "print")
    {
        function_name = "tx_print";
    }
    else if (call.name == "len")
    {
        function_name = "tx_len";
    }
    else if (call.name == "to_float")
    {
        function_name = "static_cast<double>";
    }
    else if (call.name == "is_none")
    {
        function_name = "tx_is_none";
    }
    else if (call.name == "input")
    {
        function_name = "tx_input";
    }
    else if (call.is_constructor)
    {
        function_name = cpp_struct(call.name);
    }
    const auto open = call.is_constructor ? "{" : "(";
    const auto close = call.is_constructor ? "}" : ")";
    if (call.arguments.size() <= 1)
    {
        const auto arguments = call.arguments.empty()
            ? "" : emit_expression(*call.arguments.front());
        return function_name + open + arguments + close;
    }

    const auto suffix = std::to_string(expression_index_++);
    std::string result = "([&]()\n{\n";
    std::string arguments;
    for (std::size_t index = 0; index < call.arguments.size(); ++index)
    {
        if (index != 0)
        {
            arguments += ", ";
        }
        const auto name = "tx_arg_" + suffix + "_" + std::to_string(index);
        result += "    const auto " + name + " = " +
                  emit_expression(*call.arguments[index]) + ";\n";
        arguments += name;
    }
    return result + "    return " + function_name + open + arguments +
           close + ";\n}())";
}

std::string code_generator::ordered_binary(const std::string& left,
                                           const std::string& right,
                                           std::string_view operation,
                                           bool is_function)
{
    const auto suffix = std::to_string(expression_index_++);
    const auto left_name = "tx_left_" + suffix;
    const auto right_name = "tx_right_" + suffix;
    const auto result = is_function
        ? std::string(operation) + "(" + left_name + ", " + right_name + ")"
        : "(" + left_name + " " + std::string(operation) + " " + right_name + ")";
    return "([&]()\n{\n    const auto& " + left_name + " = " + left +
           ";\n    const auto& " + right_name + " = " + right +
           ";\n    return " + result + ";\n}())";
}

std::string code_generator::emit_binary(const binary_operation& operation)
{
    const auto left = emit_expression(*operation.left);
    const auto right = emit_expression(*operation.right);
    const auto type = operation.left->type;
    if (type == value_type::none_type)
    {
        return operation.operation == token_kind::equal_equal ? "true" : "false";
    }
    if (operation.operation == token_kind::and_and)
    {
        return "(" + left + " && " + right + ")";
    }
    if (operation.operation == token_kind::or_or)
    {
        return "(" + left + " || " + right + ")";
    }
    if (type == value_type::int_type)
    {
        switch (operation.operation)
        {
        case token_kind::plus:
            return ordered_binary(left, right, "tx_add", true);
        case token_kind::minus:
            return ordered_binary(left, right, "tx_sub", true);
        case token_kind::star:
            return ordered_binary(left, right, "tx_mul", true);
        case token_kind::slash:
            return ordered_binary(left, right, "tx_div", true);
        default:
            break;
        }
    }
    if (type == value_type::float_type && operation.operation == token_kind::slash)
    {
        return ordered_binary(left, right, "tx_float_div", true);
    }
    std::string symbol;
    switch (operation.operation)
    {
    case token_kind::plus:
        symbol = "+";
        break;
    case token_kind::minus:
        symbol = "-";
        break;
    case token_kind::star:
        symbol = "*";
        break;
    case token_kind::equal_equal:
        symbol = "==";
        break;
    case token_kind::bang_equal:
        symbol = "!=";
        break;
    case token_kind::less:
        symbol = "<";
        break;
    case token_kind::less_equal:
        symbol = "<=";
        break;
    case token_kind::greater:
        symbol = ">";
        break;
    case token_kind::greater_equal:
        symbol = ">=";
        break;
    default:
        break;
    }
    return ordered_binary(left, right, symbol, false);
}

std::string code_generator::emit_array(const array_literal& literal)
{
    std::string result = "tx_array{";
    for (std::size_t index = 0; index < literal.elements.size(); ++index)
    {
        if (index != 0)
        {
            result += ", ";
        }
        result += "tx_box(" + emit_expression(*literal.elements[index]) + ")";
    }
    return result + "}";
}

std::string code_generator::emit_index(const index_expression& access)
{
    const auto suffix = std::to_string(expression_index_++);
    const auto values = "tx_values_" + suffix;
    const auto index = "tx_index_" + suffix;
    return "([&]()\n{\n    const auto& " + values + " = " +
           emit_expression(*access.object) + ";\n    const tx_int " + index +
           " = " + emit_expression(*access.index) +
           ";\n    return std::any(tx_at(" + values + ", " + index + "));\n}())";
}

std::string code_generator::emit_lvalue(const expression& item)
{
    if (const auto* name = std::get_if<name_reference>(&item.data))
    {
        return cpp_variable(name->name);
    }
    if (const auto* access = std::get_if<member_expression>(&item.data))
    {
        return "(" + emit_lvalue(*access->object) + ")." + cpp_field(access->field);
    }
    const auto& access = std::get<index_expression>(item.data);
    const auto suffix = std::to_string(expression_index_++);
    const auto values = "tx_values_" + suffix;
    const auto index = "tx_index_" + suffix;
    return "([&]() -> std::any&\n{\n    auto& " + values + " = " +
           emit_lvalue(*access.object) + ";\n    const tx_int " + index +
           " = " + emit_expression(*access.index) +
           ";\n    return tx_at(" + values + ", " + index + ");\n}())";
}

std::string code_generator::emit_expression(const expression& item)
{
    if (const auto* literal = std::get_if<integer_literal>(&item.data))
    {
        const auto first = literal->digits.find_first_not_of('0');
        const auto digits = first == std::string::npos
            ? "0" : literal->digits.substr(first);
        return "tx_int{" + digits + "}";
    }
    if (const auto* literal = std::get_if<floating_literal>(&item.data))
    {
        return literal->digits;
    }
    if (const auto* literal = std::get_if<string_literal>(&item.data))
    {
        return "std::string(" + literal->text + ")";
    }
    if (const auto* literal = std::get_if<boolean_literal>(&item.data))
    {
        return literal->value ? "true" : "false";
    }
    if (std::holds_alternative<none_literal>(item.data))
    {
        return "std::any{}";
    }
    if (const auto* literal = std::get_if<array_literal>(&item.data))
    {
        return emit_array(*literal);
    }
    if (const auto* name = std::get_if<name_reference>(&item.data))
    {
        return cpp_variable(name->name);
    }
    if (const auto* access = std::get_if<index_expression>(&item.data))
    {
        return emit_index(*access);
    }
    if (const auto* access = std::get_if<member_expression>(&item.data))
    {
        if (access->object->type == value_type::any_type)
        {
            return "tx_get_field(" + emit_expression(*access->object) +
                   ", \"" + access->field + "\")";
        }
        return "(" + emit_expression(*access->object) + ")." +
               cpp_field(access->field);
    }
    if (const auto* cast = std::get_if<cast_expression>(&item.data))
    {
        const auto value = emit_expression(*cast->value);
        if (cast->value->type == cast->target)
        {
            return value;
        }
        if (cast->value->type == value_type::any_type)
        {
            if (cast->target == value_type::int_type)
            {
                return "tx_to_int(" + value + ")";
            }
            if (cast->target == value_type::float_type)
            {
                return "tx_to_float(" + value + ")";
            }
            return "tx_to_string(" + value + ")";
        }
        if (cast->target == value_type::str_type)
        {
            if (cast->value->type == value_type::int_type)
            {
                return "tx_int_to_string(" + value + ")";
            }
            if (cast->value->type == value_type::float_type)
            {
                return "tx_float_to_string(" + value + ")";
            }
            return "tx_bool_to_string(" + value + ")";
        }
        if (cast->value->type == value_type::str_type)
        {
            return cast->target == value_type::int_type
                ? "tx_parse_int(" + value + ")"
                : "tx_parse_float(" + value + ")";
        }
        return cast->target == value_type::int_type
            ? "tx_float_to_int(" + value + ")"
            : "static_cast<double>(" + value + ")";
    }
    if (const auto* operation = std::get_if<unary_operation>(&item.data))
    {
        if (operation->is_min_int_literal)
        {
            return "std::numeric_limits<tx_int>::min()";
        }
        const auto value = emit_expression(*operation->operand);
        return operation->operation == token_kind::minus
            ? "tx_neg(" + value + ")" : "(!" + value + ")";
    }
    if (const auto* operation = std::get_if<binary_operation>(&item.data))
    {
        return emit_binary(*operation);
    }
    return emit_call(std::get<call_expression>(item.data));
}

} // namespace tx
