#include "frontend/parser/parser.hpp"

#include <utility>

namespace tx
{

struct_decl parser::parse_struct()
{
    const auto position = consume(token_kind::keyword_struct, "需要 struct").position;
    const auto name = consume(token_kind::identifier, "需要结构体名称").text;
    skip_newlines();
    (void)consume(token_kind::left_brace, "结构体需要左大括号");
    skip_newlines();
    std::vector<struct_field> fields;
    std::vector<function_decl> methods;
    while (!check(token_kind::right_brace))
    {
        if (check(token_kind::end_of_file))
        {
            throw compile_error(current().position, "结构体缺少右大括号");
        }
        if (check(token_kind::keyword_def))
        {
            auto method = parse_function(interface_mode_, true);
            if (!method.operator_kind)
            {
                throw compile_error(method.position,
                                    "struct 只允许声明运算符成员");
            }
            method.owner_class = name;
            methods.push_back(std::move(method));
        }
        else
        {
            const auto field = consume(token_kind::identifier, "需要字段名或运算符");
            (void)consume(token_kind::colon, "字段名后需要冒号");
            fields.push_back({field.text, parse_type(), field.position});
        }
        if (!check(token_kind::right_brace) && !match(token_kind::newline))
        {
            throw compile_error(current().position, "字段结束处需要换行");
        }
        skip_newlines();
    }
    (void)advance();
    return {name, std::move(fields), position, std::move(methods)};
}

function_decl parser::parse_function(bool declaration_only, bool member)
{
    const auto position = consume(token_kind::keyword_def, "需要 def").position;
    auto name = consume(token_kind::identifier, "需要函数名").text;
    std::optional<token_kind> operator_kind;
    std::string source_name = name;
    if (name == "operator" && !check(token_kind::left_paren))
    {
        if (!member)
        {
            throw compile_error(position, "operator 必须声明在 struct 或 class 内");
        }
        const auto operation = advance();
        name = operator_method_name(operation.kind);
        if (name.empty())
        {
            throw compile_error(operation.position, "不支持重载此运算符");
        }
        operator_kind = operation.kind;
        source_name = "operator " + operation.text;
    }
    (void)consume(token_kind::left_paren, "函数名后需要左括号");
    std::vector<parameter> parameters;
    if (!check(token_kind::right_paren))
    {
        bool seen_array = false;
        bool seen_dict = false;
        do
        {
            parameter_kind kind = parameter_kind::ordinary;
            if (match(token_kind::double_star))
            {
                kind = parameter_kind::variadic_dict;
                if (seen_dict)
                {
                    throw compile_error(previous().position, "只能有一个 **kwargs 参数");
                }
                seen_dict = true;
            }
            else if (match(token_kind::star))
            {
                kind = parameter_kind::variadic_array;
                if (seen_array || seen_dict)
                {
                    throw compile_error(previous().position, "*args 必须位于普通参数后、**kwargs 前");
                }
                seen_array = true;
            }
            else if (seen_array || seen_dict)
            {
                throw compile_error(current().position, "普通参数必须位于 *args 和 **kwargs 前");
            }
            const auto& parameter_name = consume(token_kind::identifier, "需要参数名");
            value_type type;
            if (kind == parameter_kind::ordinary)
            {
                (void)consume(token_kind::colon, "参数名后需要冒号");
                type = parse_type();
            }
            else
            {
                type = kind == parameter_kind::variadic_array
                    ? value_type::array_type : value_type::dict_type;
                if (match(token_kind::colon))
                {
                    const auto annotated = parse_type();
                    if (annotated != type)
                    {
                        throw compile_error(parameter_name.position,
                                            "可变参数类型必须为 " + type.name);
                    }
                }
            }
            parameters.push_back({parameter_name.text, type,
                                  parameter_name.position, kind});
        } while (match(token_kind::comma));
    }
    (void)consume(token_kind::right_paren, "参数列表缺少右括号");
    const auto return_type = match(token_kind::arrow)
        ? parse_type() : value_type::void_type;
    if (interface_mode_ || declaration_only)
    {
        if (!check(token_kind::newline) && !check(token_kind::end_of_file))
        {
            throw compile_error(current().position,
                                "接口或抽象方法声明后不能写方法体");
        }
        return {name, std::move(parameters), return_type, {}, position, true,
                source_name, {}, {}, member_access::public_access, false, false,
                false, {}, 0, operator_kind};
    }
    auto body = parse_block();
    return {name, std::move(parameters), return_type, std::move(body),
            position, false, source_name, {}, {}, member_access::public_access,
            false, false, false, {}, 0, operator_kind};
}

} // namespace tx
