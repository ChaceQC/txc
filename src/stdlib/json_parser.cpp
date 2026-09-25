#include "stdlib/json.hpp"
#include "stdlib/json_utf8.hpp"

#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

namespace tx_generated
{
namespace
{

bool is_digit(char value) noexcept
{
    return value >= '0' && value <= '9';
}

int hex_digit(char value) noexcept
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

class json_parser
{
public:
    explicit json_parser(std::string_view text) : text_(text)
    {
    }

    [[nodiscard]] std::any parse()
    {
        skip_space();
        if (offset_ == text_.size())
        {
            fail("empty_input", "JSON 文本不能为空");
        }
        auto result = parse_value(0);
        skip_space();
        if (offset_ != text_.size())
        {
            fail("invalid_syntax", "JSON 根值后存在多余内容");
        }
        return result;
    }

private:
    [[noreturn]] void fail(const char* code, std::string_view reason) const
    {
        std::size_t line = 1;
        std::size_t column = 1;
        for (std::size_t index = 0; index < offset_; ++index)
        {
            if (text_[index] == '\n')
            {
                ++line;
                column = 1;
            }
            else if ((static_cast<unsigned char>(text_[index]) & 0xc0) != 0x80)
            {
                ++column;
            }
        }
        throw runtime_failure({tx::error_kind::parse, code,
            "JSON 第 " + std::to_string(line) + " 行第 " +
            std::to_string(column) + " 列（字节偏移 " +
            std::to_string(offset_) + "）：" + std::string(reason)});
    }

    void skip_space() noexcept
    {
        while (offset_ < text_.size() &&
               (text_[offset_] == ' ' || text_[offset_] == '\t' ||
                text_[offset_] == '\r' || text_[offset_] == '\n'))
        {
            ++offset_;
        }
    }

    [[nodiscard]] bool take(char expected) noexcept
    {
        if (offset_ < text_.size() && text_[offset_] == expected)
        {
            ++offset_;
            return true;
        }
        return false;
    }

    [[nodiscard]] std::any parse_value(std::size_t depth)
    {
        if (depth > 128)
        {
            fail("depth_limit", "JSON 嵌套超过 128 层");
        }
        if (offset_ == text_.size())
        {
            fail("invalid_syntax", "缺少 JSON 值");
        }
        switch (text_[offset_])
        {
        case '"': return parse_string();
        case '[': return parse_array(depth);
        case '{': return parse_object(depth);
        case 't': return parse_literal("true", true);
        case 'f': return parse_literal("false", false);
        case 'n': return parse_literal("null", std::any{});
        default:
            if (text_[offset_] == '-' || is_digit(text_[offset_]))
            {
                return parse_number();
            }
            fail("invalid_syntax", "此处需要 JSON 值");
        }
    }

    [[nodiscard]] std::any parse_literal(std::string_view literal,
                                         std::any value)
    {
        if (text_.substr(offset_, literal.size()) != literal)
        {
            fail("invalid_syntax", "JSON 字面量不完整或无效");
        }
        offset_ += literal.size();
        return value;
    }

    [[nodiscard]] tx_array parse_array(std::size_t depth)
    {
        ++offset_;
        tx_array result;
        skip_space();
        if (take(']'))
        {
            return result;
        }
        while (true)
        {
            skip_space();
            result.push_back(parse_value(depth + 1));
            skip_space();
            if (take(']'))
            {
                return result;
            }
            if (!take(','))
            {
                fail("invalid_syntax", "数组元素后需要逗号或右方括号");
            }
        }
    }

    [[nodiscard]] tx_dict parse_object(std::size_t depth)
    {
        ++offset_;
        tx_dict result;
        skip_space();
        if (take('}'))
        {
            return result;
        }
        while (true)
        {
            skip_space();
            if (offset_ == text_.size() || text_[offset_] != '"')
            {
                fail("invalid_syntax", "对象字段名必须是字符串");
            }
            auto key = parse_string();
            skip_space();
            if (!take(':'))
            {
                fail("invalid_syntax", "对象字段名后需要冒号");
            }
            skip_space();
            result.emplace_back(std::move(key), parse_value(depth + 1));
            skip_space();
            if (take('}'))
            {
                return result;
            }
            if (!take(','))
            {
                fail("invalid_syntax", "对象字段后需要逗号或右花括号");
            }
        }
    }

    [[nodiscard]] std::uint32_t parse_hex_quad()
    {
        std::uint32_t value = 0;
        for (int index = 0; index < 4; ++index)
        {
            if (offset_ == text_.size() || hex_digit(text_[offset_]) < 0)
            {
                fail("invalid_escape", "\\u 后需要四位十六进制数字");
            }
            value = value * 16 + static_cast<std::uint32_t>(
                hex_digit(text_[offset_++]));
        }
        return value;
    }

    void parse_unicode_escape(std::string& output)
    {
        std::uint32_t codepoint = parse_hex_quad();
        if (codepoint >= 0xd800 && codepoint <= 0xdbff)
        {
            if (!take('\\') || !take('u'))
            {
                fail("invalid_escape", "高代理项后需要低代理项转义");
            }
            const auto low = parse_hex_quad();
            if (low < 0xdc00 || low > 0xdfff)
            {
                fail("invalid_escape", "Unicode 低代理项无效");
            }
            codepoint = 0x10000 + ((codepoint - 0xd800) << 10) +
                        (low - 0xdc00);
        }
        else if (codepoint >= 0xdc00 && codepoint <= 0xdfff)
        {
            fail("invalid_escape", "Unicode 低代理项缺少高代理项");
        }
        json_detail::append_utf8(output, codepoint);
    }

    void parse_escape(std::string& output)
    {
        if (offset_ == text_.size())
        {
            fail("invalid_escape", "反斜杠后缺少转义字符");
        }
        switch (text_[offset_++])
        {
        case '"': output.push_back('"'); break;
        case '\\': output.push_back('\\'); break;
        case '/': output.push_back('/'); break;
        case 'b': output.push_back('\b'); break;
        case 'f': output.push_back('\f'); break;
        case 'n': output.push_back('\n'); break;
        case 'r': output.push_back('\r'); break;
        case 't': output.push_back('\t'); break;
        case 'u': parse_unicode_escape(output); break;
        default: fail("invalid_escape", "JSON 字符串转义无效");
        }
    }

    [[nodiscard]] std::string parse_string()
    {
        ++offset_;
        std::string result;
        while (offset_ < text_.size())
        {
            const auto byte = static_cast<unsigned char>(text_[offset_]);
            if (byte == '"')
            {
                ++offset_;
                return result;
            }
            if (byte == '\\')
            {
                ++offset_;
                parse_escape(result);
                continue;
            }
            if (byte < 0x20)
            {
                fail("invalid_syntax", "JSON 字符串不能含未转义控制字符");
            }
            const auto width = json_detail::utf8_width(text_, offset_);
            if (width == 0)
            {
                fail("invalid_utf8", "JSON 字符串包含无效 UTF-8");
            }
            result.append(text_.substr(offset_, width));
            offset_ += width;
        }
        fail("invalid_syntax", "JSON 字符串缺少结束引号");
    }

    [[nodiscard]] std::any parse_number()
    {
        const auto start = offset_;
        (void)take('-');
        if (offset_ == text_.size() || !is_digit(text_[offset_]))
        {
            fail("invalid_syntax", "负号后需要数字");
        }
        if (take('0'))
        {
            if (offset_ < text_.size() && is_digit(text_[offset_]))
            {
                fail("invalid_syntax", "JSON 数字不能有前导零");
            }
        }
        else
        {
            while (offset_ < text_.size() && is_digit(text_[offset_]))
            {
                ++offset_;
            }
        }
        bool floating = false;
        if (take('.'))
        {
            floating = true;
            if (offset_ == text_.size() || !is_digit(text_[offset_]))
            {
                fail("invalid_syntax", "小数点后需要数字");
            }
            while (offset_ < text_.size() && is_digit(text_[offset_]))
            {
                ++offset_;
            }
        }
        if (take('e') || take('E'))
        {
            floating = true;
            if (!take('+'))
            {
                (void)take('-');
            }
            if (offset_ == text_.size() || !is_digit(text_[offset_]))
            {
                fail("invalid_syntax", "指数符号后需要数字");
            }
            while (offset_ < text_.size() && is_digit(text_[offset_]))
            {
                ++offset_;
            }
        }
        const auto first = text_.data() + start;
        const auto last = text_.data() + offset_;
        if (floating)
        {
            double value = 0;
            const auto [end, error] = std::from_chars(first, last, value,
                                                      std::chars_format::general);
            if (error == std::errc::result_out_of_range || !std::isfinite(value))
            {
                fail("out_of_range", "JSON 浮点数超出有限 float 范围");
            }
            if (error != std::errc{} || end != last)
            {
                fail("invalid_syntax", "JSON 浮点数无效");
            }
            return value;
        }
        std::int64_t value = 0;
        const auto [end, error] = std::from_chars(first, last, value);
        if (error == std::errc::result_out_of_range)
        {
            fail("out_of_range", "JSON 整数超出 int 范围");
        }
        if (error != std::errc{} || end != last)
        {
            fail("invalid_syntax", "JSON 整数无效");
        }
        return value;
    }

    std::string_view text_;
    std::size_t offset_ = 0;
};

} // namespace

std::any json_parse(std::string_view text)
{
    return json_parser(text).parse();
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
