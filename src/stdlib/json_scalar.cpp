#include "stdlib/json_scalar.hpp"
#include "stdlib/json_utf8.hpp"

#include <charconv>
#include <cmath>

namespace tx_generated::json_detail
{
namespace
{

[[noreturn]] void scalar_error(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::runtime, code, message});
}

} // namespace

void append_string(output_buffer& output, std::string_view text)
{
    constexpr char hex[] = "0123456789abcdef";
    output.push_back('"');
    for (std::size_t offset = 0; offset < text.size();)
    {
        const auto byte = static_cast<unsigned char>(text[offset]);
        if (byte >= 0x80)
        {
            const auto width = json_detail::utf8_width(text, offset);
            if (width == 0)
            {
                scalar_error("invalid_utf8", "JSON 字符串包含无效 UTF-8");
            }
            output.append(text.substr(offset, width));
            offset += width;
            continue;
        }
        ++offset;
        switch (byte)
        {
        case '"': output += "\\\""; break;
        case '\\': output += "\\\\"; break;
        case '\b': output += "\\b"; break;
        case '\f': output += "\\f"; break;
        case '\n': output += "\\n"; break;
        case '\r': output += "\\r"; break;
        case '\t': output += "\\t"; break;
        default:
            if (byte < 0x20)
            {
                output += "\\u00";
                output.push_back(hex[byte >> 4]);
                output.push_back(hex[byte & 0x0f]);
            }
            else
            {
                output.push_back(static_cast<char>(byte));
            }
        }
    }
    output.push_back('"');
}

void append_float(output_buffer& output, double value)
{
    if (!std::isfinite(value))
    {
        scalar_error("non_finite", "JSON 不能序列化 NaN 或无穷大");
    }
    char buffer[128];
    const auto [end, error] = std::to_chars(
        buffer, buffer + sizeof(buffer), value, std::chars_format::general);
    if (error != std::errc{})
    {
        scalar_error("operation_failed", "JSON 浮点数序列化失败");
    }
    const std::string_view text(buffer, end);
    output.append(text);
    if (text.find_first_of(".eE") == std::string_view::npos)
    {
        output += ".0";
    }
}

} // namespace tx_generated::json_detail
