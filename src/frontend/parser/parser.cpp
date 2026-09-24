#include "frontend/parser/parser.hpp"

#include <memory>
#include <utility>

namespace tx
{

parser::parser(std::vector<token> tokens, bool interface_mode)
    : tokens_(std::move(tokens)), interface_mode_(interface_mode)
{
}

const token& parser::current() const
{
    return tokens_[index_];
}

const token& parser::previous() const
{
    return tokens_[index_ - 1];
}

const token& parser::advance()
{
    if (!check(token_kind::end_of_file))
    {
        ++index_;
    }
    return previous();
}

bool parser::check(token_kind kind) const
{
    return current().kind == kind;
}

bool parser::match(token_kind kind)
{
    if (!check(kind))
    {
        return false;
    }
    (void)advance();
    return true;
}

const token& parser::consume(token_kind kind, const char* message)
{
    if (!check(kind))
    {
        throw compile_error(current().position, message);
    }
    return advance();
}

void parser::skip_newlines()
{
    while (match(token_kind::newline))
    {
    }
}

value_type parser::parse_type()
{
    if (match(token_kind::keyword_int))
    {
        return value_type::int_type;
    }
    if (match(token_kind::keyword_bool))
    {
        return value_type::bool_type;
    }
    if (match(token_kind::keyword_float))
    {
        return value_type::float_type;
    }
    if (match(token_kind::keyword_str))
    {
        return value_type::str_type;
    }
    if (match(token_kind::keyword_array))
    {
        return value_type::array_type;
    }
    if (match(token_kind::keyword_dict))
    {
        return value_type::dict_type;
    }
    if (match(token_kind::identifier))
    {
        std::string name = previous().text;
        while (match(token_kind::dot))
        {
            name += "." + consume(token_kind::identifier, "点号后需要类型名").text;
        }
        return value_type(std::move(name));
    }
    throw compile_error(current().position, "需要类型名称");
}

import_decl parser::parse_import()
{
    const auto position = consume(token_kind::keyword_import, "需要 import").position;
    const auto& path = consume(token_kind::string_literal, "import 后需要路径字符串");
    const auto value = path.text.substr(1, path.text.size() - 2);
    if (value.empty() || value.find('\\') != std::string::npos)
    {
        throw compile_error(path.position, "导入路径不能为空，请使用正斜杠且不要转义");
    }
    std::string alias;
    if (match(token_kind::keyword_as))
    {
        alias = consume(token_kind::identifier, "as 后需要模块别名").text;
    }
    return {value, alias, position};
}

struct_decl parser::parse_struct()
{
    const auto position = consume(token_kind::keyword_struct, "需要 struct").position;
    const auto name = consume(token_kind::identifier, "需要结构体名称").text;
    (void)consume(token_kind::left_brace, "结构体需要左大括号");
    skip_newlines();
    std::vector<struct_field> fields;
    while (!check(token_kind::right_brace))
    {
        if (check(token_kind::end_of_file))
        {
            throw compile_error(current().position, "结构体缺少右大括号");
        }
        const auto field = consume(token_kind::identifier, "需要字段名");
        (void)consume(token_kind::colon, "字段名后需要冒号");
        fields.push_back({field.text, parse_type(), field.position});
        if (!check(token_kind::right_brace) && !match(token_kind::newline))
        {
            throw compile_error(current().position, "字段结束处需要换行");
        }
        skip_newlines();
    }
    (void)advance();
    return {name, std::move(fields), position};
}

function_decl parser::parse_function()
{
    const auto position = consume(token_kind::keyword_def, "需要 def").position;
    const auto name = consume(token_kind::identifier, "需要函数名").text;
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
    if (interface_mode_)
    {
        if (!check(token_kind::newline) && !check(token_kind::end_of_file))
        {
            throw compile_error(current().position, ".txh 只允许函数声明");
        }
        return {name, std::move(parameters), return_type, {}, position, true,
                name, {}};
    }
    auto body = parse_block();
    return {name, std::move(parameters), return_type, std::move(body),
            position, false, name, {}};
}

std::vector<stmt_ptr> parser::parse_block()
{
    (void)consume(token_kind::left_brace, "需要左大括号");
    skip_newlines();
    std::vector<stmt_ptr> body;
    while (!check(token_kind::right_brace))
    {
        if (check(token_kind::end_of_file))
        {
            throw compile_error(current().position, "代码块缺少右大括号");
        }
        body.push_back(parse_statement());
        if (!check(token_kind::right_brace) && !match(token_kind::newline))
        {
            throw compile_error(current().position, "语句结束处需要换行");
        }
        skip_newlines();
    }
    (void)advance();
    return body;
}

bool parser::looks_like_declaration() const
{
    const auto kind = current().kind;
    if (kind == token_kind::keyword_auto || kind == token_kind::keyword_int ||
        kind == token_kind::keyword_bool || kind == token_kind::keyword_float ||
        kind == token_kind::keyword_str || kind == token_kind::keyword_dict)
    {
        return true;
    }
    if (kind != token_kind::identifier)
    {
        return false;
    }
    std::size_t next = index_ + 1;
    while (next + 1 < tokens_.size() &&
           tokens_[next].kind == token_kind::dot &&
           tokens_[next + 1].kind == token_kind::identifier)
    {
        next += 2;
    }
    return next + 1 < tokens_.size() &&
           tokens_[next].kind == token_kind::identifier &&
           tokens_[next + 1].kind == token_kind::equal;
}

stmt_ptr parser::parse_for(source_pos position)
{
    const auto name = consume(token_kind::identifier, "for 后需要循环变量").text;
    (void)consume(token_kind::keyword_in, "循环变量后需要 in");
    auto values = parse_expression();
    if (match(token_kind::range_inclusive))
    {
        auto last = parse_expression();
        auto body = parse_block();
        return std::make_unique<statement>(
            position, for_loop{name, std::move(values), std::move(last), std::move(body)});
    }
    auto body = parse_block();
    return std::make_unique<statement>(
        position, for_each{name, std::move(values), std::move(body)});
}

stmt_ptr parser::parse_if(source_pos position)
{
    auto condition = parse_expression();
    auto then_body = parse_block();
    const auto saved_index = index_;
    skip_newlines();
    const bool has_else = match(token_kind::keyword_else);
    std::vector<stmt_ptr> else_body;
    if (has_else)
    {
        else_body = parse_block();
    }
    else
    {
        index_ = saved_index;
    }
    return std::make_unique<statement>(
        position, if_statement{std::move(condition), std::move(then_body),
                               std::move(else_body), has_else});
}

stmt_ptr parser::parse_while(source_pos position)
{
    auto condition = parse_expression();
    auto body = parse_block();
    return std::make_unique<statement>(
        position, while_statement{std::move(condition), std::move(body)});
}

stmt_ptr parser::parse_statement()
{
    const auto position = current().position;
    if (match(token_kind::keyword_for))
    {
        return parse_for(position);
    }
    if (match(token_kind::keyword_if))
    {
        return parse_if(position);
    }
    if (match(token_kind::keyword_while))
    {
        return parse_while(position);
    }
    if (match(token_kind::keyword_return))
    {
        expr_ptr value;
        if (!check(token_kind::newline) && !check(token_kind::right_brace))
        {
            value = parse_expression();
        }
        return std::make_unique<statement>(
            position, return_statement{std::move(value)});
    }
    if (check(token_kind::identifier) &&
        tokens_[index_ + 1].kind == token_kind::comma)
    {
        std::vector<std::string> names;
        names.push_back(advance().text);
        (void)consume(token_kind::comma, "解包变量名之间需要逗号");
        do
        {
            names.push_back(consume(token_kind::identifier,
                                    "解包赋值需要变量名").text);
        } while (match(token_kind::comma));
        (void)consume(token_kind::equal, "解包赋值需要 =");
        return std::make_unique<statement>(
            position, unpack_assignment{std::move(names), {}, parse_expression()});
    }
    if (match(token_kind::keyword_array))
    {
        expr_ptr length;
        if (match(token_kind::left_bracket))
        {
            length = parse_expression();
            (void)consume(token_kind::right_bracket, "数组长度缺少右方括号");
        }
        const auto name = consume(token_kind::identifier, "数组声明需要变量名").text;
        expr_ptr initializer;
        if (match(token_kind::equal))
        {
            initializer = parse_expression();
        }
        else if (length == nullptr)
        {
            throw compile_error(current().position, "未指定长度的数组需要初值");
        }
        return std::make_unique<statement>(
            position, variable_declaration{name, value_type::array_type,
                                           std::move(initializer), std::move(length)});
    }
    if (looks_like_declaration())
    {
        const bool inferred = match(token_kind::keyword_auto);
        const auto declared_type = inferred
            ? std::optional<value_type>{}
            : std::optional<value_type>{parse_type()};
        const auto name = consume(token_kind::identifier, "声明需要变量名").text;
        (void)consume(token_kind::equal, "变量声明需要初值");
        return std::make_unique<statement>(
            position, variable_declaration{name, declared_type, parse_expression(), nullptr});
    }
    auto target = parse_expression();
    if (match(token_kind::equal) || match(token_kind::plus_equal))
    {
        const auto operation = previous().kind;
        return std::make_unique<statement>(
            position, variable_assignment{std::move(target), operation, parse_expression()});
    }
    return std::make_unique<statement>(
        position, expression_statement{std::move(target)});
}

program parser::parse_program()
{
    program result;
    skip_newlines();
    while (!check(token_kind::end_of_file))
    {
        if (check(token_kind::keyword_import))
        {
            result.imports.push_back(parse_import());
            if (!check(token_kind::newline) && !check(token_kind::end_of_file))
            {
                throw compile_error(current().position, "import 后需要换行");
            }
        }
        else if (check(token_kind::keyword_struct))
        {
            result.structs.push_back(parse_struct());
        }
        else
        {
            result.functions.push_back(parse_function());
        }
        skip_newlines();
    }
    return result;
}

} // namespace tx
