#include "stdlib/json_parser.hpp"

#include <charconv>
#include <cmath>
#include <utility>

namespace tx_generated
{
namespace
{

bool is_digit(int value)
{
    return value >= '0' && value <= '9';
}

} // namespace

json_parser::json_parser(format_input& input, std::size_t max_depth,
                         bool reject_duplicate_keys)
    : input_(input), max_depth_(max_depth),
      reject_duplicate_keys_(reject_duplicate_keys)
{
}

std::any json_parser::parse()
{
    skip_space();
    if (input_.peek() < 0)
    {
        input_.fail("empty_input", "JSON 文本不能为空");
    }
    auto value = parse_value(0);
    require_end();
    return value;
}

void json_parser::skip_space()
{
    for (;;)
    {
        const auto byte = input_.peek();
        if (byte != ' ' && byte != '\t' && byte != '\r' && byte != '\n')
        {
            return;
        }
        (void)input_.get();
    }
}

void json_parser::require_end()
{
    skip_space();
    if (input_.peek() >= 0)
    {
        input_.fail("invalid_syntax", "JSON 根值后存在多余内容");
    }
}

std::any json_parser::parse_value(std::size_t depth)
{
    if (depth > max_depth_)
    {
        input_.fail("depth_limit", "JSON 嵌套超过深度上限");
    }
    switch (input_.peek())
    {
    case '"': return parse_string();
    case '[': return parse_array(depth);
    case '{': return parse_object(depth);
    case 't': return parse_literal("true", true);
    case 'f': return parse_literal("false", false);
    case 'n': return parse_literal("null", std::any{});
    default:
        if (input_.peek() == '-' || is_digit(input_.peek()))
        {
            return parse_number();
        }
        input_.fail("invalid_syntax", "此处需要 JSON 值");
    }
}

std::any json_parser::parse_literal(std::string_view literal, std::any value)
{
    for (const auto byte : literal)
    {
        if (!input_.take(byte))
        {
            input_.fail("invalid_syntax", "JSON 字面量不完整或无效");
        }
    }
    return value;
}

tx_array json_parser::parse_array(std::size_t depth)
{
    (void)input_.get();
    tx_array result;
    skip_space();
    if (input_.take(']'))
    {
        return result;
    }
    while (true)
    {
        skip_space();
        result.push_back(parse_value(depth + 1));
        skip_space();
        if (input_.take(']'))
        {
            return result;
        }
        if (!input_.take(','))
        {
            input_.fail("invalid_syntax", "数组元素后需要逗号或右方括号");
        }
    }
}

tx_dict json_parser::parse_object(std::size_t depth)
{
    (void)input_.get();
    tx_dict result;
    skip_space();
    if (input_.take('}'))
    {
        return result;
    }
    while (true)
    {
        skip_space();
        if (input_.peek() != '"')
        {
            input_.fail("invalid_syntax", "对象字段名必须是字符串");
        }
        auto key = parse_string();
        if (reject_duplicate_keys_ &&
            result.find_value(std::string_view(key)) != nullptr)
        {
            input_.fail("duplicate_key", "serde JSON 对象包含重复字段名");
        }
        skip_space();
        if (!input_.take(':'))
        {
            input_.fail("invalid_syntax", "对象字段名后需要冒号");
        }
        skip_space();
        result.emplace_back(std::move(key), parse_value(depth + 1));
        skip_space();
        if (input_.take('}'))
        {
            return result;
        }
        if (!input_.take(','))
        {
            input_.fail("invalid_syntax", "对象字段后需要逗号或右花括号");
        }
    }
}

std::any json_parser::parse_number()
{
    return std::visit([](auto value) -> std::any
    {
        return value;
    }, parse_numeric());
}

std::variant<std::int64_t, double> json_parser::parse_numeric()
{
    std::string token;
    if (input_.take('-'))
    {
        token += '-';
    }
    if (!is_digit(input_.peek()))
    {
        input_.fail("invalid_syntax", "负号后需要数字");
    }
    token += input_.get();
    if (token.back() == '0' && is_digit(input_.peek()))
    {
        input_.fail("invalid_syntax", "JSON 数字不能有前导零");
    }
    while (is_digit(input_.peek()))
    {
        token += input_.get();
    }
    bool floating = false;
    for (const auto part : {'.', 'e'})
    {
        if (input_.peek() != part && !(part == 'e' && input_.peek() == 'E'))
        {
            continue;
        }
        floating = true;
        token += input_.get();
        if (part == 'e' && (input_.peek() == '+' || input_.peek() == '-'))
        {
            token += input_.get();
        }
        if (!is_digit(input_.peek()))
        {
            input_.fail("invalid_syntax", "小数点或指数符号后需要数字");
        }
        while (is_digit(input_.peek()))
        {
            token += input_.get();
        }
    }
    if (floating)
    {
        double value = 0;
        const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
        if (parsed.ec == std::errc::result_out_of_range || !std::isfinite(value))
        {
            input_.fail("out_of_range", "JSON 浮点数超出有限 float 范围");
        }
        if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size())
        {
            input_.fail("invalid_syntax", "JSON 浮点数无效");
        }
        return value;
    }
    std::int64_t value = 0;
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
    if (parsed.ec == std::errc::result_out_of_range)
    {
        input_.fail("out_of_range", "JSON 整数超出 int 范围");
    }
    return value;
}

std::any json_parse(std::string_view text)
{
    format_input input(text, "JSON");
    return json_parser(input).parse();
}

std::any json_parse_unique(std::string_view text)
{
    format_input input(text, "JSON");
    return json_parser(input, 128, true).parse();
}

operation_result<std::any> json_try_parse(std::string_view text)
{
    try
    {
        return {true, json_parse(text), {}};
    }
    catch (const runtime_failure& error)
    {
        return {false, {}, error.error()};
    }
}

} // namespace tx_generated
