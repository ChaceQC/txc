#include "stdlib/json_parser.hpp"
#include "stdlib/json_utf8.hpp"

namespace tx_generated
{
namespace
{

int hex_digit(int value)
{
    if (value >= '0' && value <= '9')
    {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f')
    {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F')
    {
        return value - 'A' + 10;
    }
    return -1;
}

} // namespace

std::uint32_t json_parser::parse_hex_quad()
{
    std::uint32_t value = 0;
    for (int index = 0; index < 4; ++index)
    {
        const auto digit = hex_digit(input_.peek());
        if (digit < 0)
        {
            input_.fail("invalid_escape", "\\u 后需要四位十六进制数字");
        }
        (void)input_.get();
        value = value * 16 + static_cast<std::uint32_t>(digit);
    }
    return value;
}

void json_parser::parse_unicode_escape(std::string& output)
{
    auto codepoint = parse_hex_quad();
    if (codepoint >= 0xd800 && codepoint <= 0xdbff)
    {
        if (!input_.take('\\') || !input_.take('u'))
        {
            input_.fail("invalid_escape", "高代理项后需要低代理项转义");
        }
        const auto low = parse_hex_quad();
        if (low < 0xdc00 || low > 0xdfff)
        {
            input_.fail("invalid_escape", "Unicode 低代理项无效");
        }
        codepoint = 0x10000 + ((codepoint - 0xd800) << 10) + (low - 0xdc00);
    }
    else if (codepoint >= 0xdc00 && codepoint <= 0xdfff)
    {
        input_.fail("invalid_escape", "Unicode 低代理项缺少高代理项");
    }
    json_detail::append_utf8(output, codepoint);
}

void json_parser::parse_escape(std::string& output)
{
    if (input_.peek() < 0)
    {
        input_.fail("invalid_escape", "反斜杠后缺少转义字符");
    }
    switch (input_.get())
    {
    case '"': output += '"'; break;
    case '\\': output += '\\'; break;
    case '/': output += '/'; break;
    case 'b': output += '\b'; break;
    case 'f': output += '\f'; break;
    case 'n': output += '\n'; break;
    case 'r': output += '\r'; break;
    case 't': output += '\t'; break;
    case 'u': parse_unicode_escape(output); break;
    default: input_.fail("invalid_escape", "JSON 字符串转义无效");
    }
}

std::string json_parser::parse_string()
{
    (void)input_.get();
    std::string output;
    while (input_.peek() >= 0)
    {
        const auto plain = input_.take_json_ascii();
        if (!plain.empty())
        {
            output.append(plain);
            continue;
        }
        const auto lead = input_.peek();
        if (lead == '"')
        {
            (void)input_.get();
            return output;
        }
        if (lead == '\\')
        {
            (void)input_.get();
            parse_escape(output);
            continue;
        }
        if (lead < 0x20)
        {
            input_.fail("invalid_syntax", "JSON 字符串不能含未转义控制字符");
        }
        const std::size_t width = lead < 0x80 ? 1 : lead < 0xe0 ? 2 : lead < 0xf0 ? 3 : 4;
        std::string scalar;
        for (std::size_t index = 0; index < width && input_.peek(index) >= 0; ++index)
        {
            scalar += static_cast<char>(input_.peek(index));
        }
        if (json_detail::utf8_width(scalar, 0) == 0)
        {
            input_.fail("invalid_utf8", "JSON 字符串包含无效 UTF-8");
        }
        for (std::size_t index = 0; index < width; ++index)
        {
            output += input_.get();
        }
    }
    input_.fail("invalid_syntax", "JSON 字符串缺少结束引号");
}

} // namespace tx_generated
