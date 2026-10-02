#include "stdlib/native_gui/core/unicode.hpp"

#include <stdexcept>

namespace tx::ui
{
bool unicode_whitespace(char32_t scalar) noexcept
{
    return (scalar >= 0x9 && scalar <= 0xd) || scalar == 0x20 || scalar == 0x85 || scalar == 0xa0 ||
        scalar == 0x1680 || (scalar >= 0x2000 && scalar <= 0x200a) || scalar == 0x2028 || scalar == 0x2029 ||
        scalar == 0x202f || scalar == 0x205f || scalar == 0x3000;
}

unicode_text decode_utf8(std::string_view text)
{
    unicode_text result;
    for (std::size_t index = 0; index < text.size();)
    {
        result.byte_offsets.push_back(index);
        const auto first = static_cast<unsigned char>(text[index++]);
        char32_t value = 0;
        unsigned remaining = 0;
        char32_t minimum = 0;
        if (first < 0x80)
        {
            value = first;
        }
        else if (first >= 0xc2 && first <= 0xdf)
        {
            value = first & 0x1f;
            remaining = 1;
            minimum = 0x80;
        }
        else if (first >= 0xe0 && first <= 0xef)
        {
            value = first & 0xf;
            remaining = 2;
            minimum = 0x800;
        }
        else if (first >= 0xf0 && first <= 0xf4)
        {
            value = first & 7;
            remaining = 3;
            minimum = 0x10000;
        }
        else
        {
            throw std::invalid_argument("UTF-8 起始字节非法");
        }
        for (unsigned continuation = 0; continuation < remaining; ++continuation)
        {
            if (index == text.size())
            {
                throw std::invalid_argument("UTF-8 字符截断");
            }
            const auto byte = static_cast<unsigned char>(text[index++]);
            if ((byte & 0xc0) != 0x80)
            {
                throw std::invalid_argument("UTF-8 延续字节非法");
            }
            value = (value << 6) | (byte & 0x3f);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
        {
            throw std::invalid_argument("UTF-8 编码不是最短形式或包含非法 Unicode 标量");
        }
        result.scalars.push_back(value);
    }
    result.byte_offsets.push_back(text.size());
    return result;
}

std::string encode_utf8(std::u32string_view text)
{
    std::string result;
    for (const auto value : text)
    {
        if (value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
        {
            throw std::invalid_argument("文本包含非法 Unicode 标量");
        }
        if (value < 0x80)
        {
            result += static_cast<char>(value);
        }
        else if (value < 0x800)
        {
            result += static_cast<char>(0xc0 | (value >> 6));
            result += static_cast<char>(0x80 | (value & 0x3f));
        }
        else
        {
            if (value < 0x10000)
            {
                result += static_cast<char>(0xe0 | (value >> 12));
            }
            else
            {
                result += static_cast<char>(0xf0 | (value >> 18));
                result += static_cast<char>(0x80 | ((value >> 12) & 0x3f));
            }
            result += static_cast<char>(0x80 | ((value >> 6) & 0x3f));
            result += static_cast<char>(0x80 | (value & 0x3f));
        }
    }
    return result;
}
}
