#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace tx_generated::json_detail
{

// 返回当前 UTF-8 标量的字节宽度；零表示无效编码。
[[nodiscard]] inline std::size_t utf8_width(std::string_view text,
                                             std::size_t offset) noexcept
{
    const auto lead = static_cast<unsigned char>(text[offset]);
    std::size_t width = 0;
    if (lead <= 0x7f)
    {
        return 1;
    }
    if (lead >= 0xc2 && lead <= 0xdf)
    {
        width = 2;
    }
    else if (lead >= 0xe0 && lead <= 0xef)
    {
        width = 3;
    }
    else if (lead >= 0xf0 && lead <= 0xf4)
    {
        width = 4;
    }
    if (width == 0 || width > text.size() - offset)
    {
        return 0;
    }
    for (std::size_t index = 1; index < width; ++index)
    {
        const auto byte = static_cast<unsigned char>(text[offset + index]);
        if (byte < 0x80 || byte > 0xbf)
        {
            return 0;
        }
    }
    const auto second = static_cast<unsigned char>(text[offset + 1]);
    if ((lead == 0xe0 && second < 0xa0) ||
        (lead == 0xed && second >= 0xa0) ||
        (lead == 0xf0 && second < 0x90) ||
        (lead == 0xf4 && second >= 0x90))
    {
        return 0;
    }
    return width;
}

inline void append_utf8(std::string& output, std::uint32_t codepoint)
{
    if (codepoint <= 0x7f)
    {
        output.push_back(static_cast<char>(codepoint));
    }
    else if (codepoint <= 0x7ff)
    {
        output.push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    else if (codepoint <= 0xffff)
    {
        output.push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    else
    {
        output.push_back(static_cast<char>(0xf0 | (codepoint >> 18)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
}

} // namespace tx_generated::json_detail
