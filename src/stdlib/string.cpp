#include "stdlib/stdlib.hpp"

#include <limits>
#include <stdexcept>
#include <string_view>
#include <typeinfo>

namespace tx_generated
{
namespace
{

std::size_t next_utf8(std::string_view text, std::size_t offset)
{
    const auto lead = static_cast<unsigned char>(text[offset]);
    std::size_t width = 0;
    if (lead <= 0x7f)
    {
        width = 1;
    }
    else if (lead >= 0xc2 && lead <= 0xdf)
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
        throw std::runtime_error("字符串包含无效 UTF-8");
    }
    for (std::size_t index = 1; index < width; ++index)
    {
        const auto byte = static_cast<unsigned char>(text[offset + index]);
        if (byte < 0x80 || byte > 0xbf)
        {
            throw std::runtime_error("字符串包含无效 UTF-8");
        }
    }
    if (width >= 3)
    {
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        if ((lead == 0xe0 && second < 0xa0) ||
            (lead == 0xed && second >= 0xa0) ||
            (lead == 0xf0 && second < 0x90) ||
            (lead == 0xf4 && second >= 0x90))
        {
            throw std::runtime_error("字符串包含无效 UTF-8");
        }
    }
    return offset + width;
}

std::size_t byte_offset(std::string_view text, tx_int character_index)
{
    if (character_index < 0)
    {
        throw std::out_of_range("字符串位置不能为负");
    }
    std::size_t offset = 0;
    for (tx_int index = 0; index < character_index; ++index)
    {
        if (offset == text.size())
        {
            throw std::out_of_range("字符串位置越界");
        }
        offset = next_utf8(text, offset);
    }
    return offset;
}

bool is_ascii_space(char value)
{
    return value == ' ' || value == '\t' || value == '\n' ||
           value == '\r' || value == '\v' || value == '\f';
}

} // namespace

tx_int tx_len(const std::string& text)
{
    tx_int count = 0;
    for (std::size_t offset = 0; offset < text.size();)
    {
        offset = next_utf8(text, offset);
        if (count == std::numeric_limits<tx_int>::max())
        {
            throw std::overflow_error("字符串长度超出 int 范围");
        }
        ++count;
    }
    return count;
}

bool tx_fn_contains(std::string text, std::string part)
{
    return text.find(part) != std::string::npos;
}

bool tx_fn_starts_with(std::string text, std::string prefix)
{
    return text.starts_with(prefix);
}

bool tx_fn_ends_with(std::string text, std::string suffix)
{
    return text.ends_with(suffix);
}

tx_int tx_fn_find(std::string text, std::string part)
{
    (void)tx_len(text);
    const auto position = text.find(part);
    if (position == std::string::npos)
    {
        return -1;
    }
    return tx_len(text.substr(0, position));
}

std::string tx_fn_slice(std::string text, tx_int start, tx_int end)
{
    const auto length = tx_len(text);
    if (start < 0 || end < start || end > length)
    {
        throw std::out_of_range("字符串切片范围无效");
    }
    const auto first = byte_offset(text, start);
    const auto last = byte_offset(text, end);
    return text.substr(first, last - first);
}

std::string tx_fn_replace(std::string text, std::string old, std::string replacement)
{
    if (old.empty())
    {
        throw std::runtime_error("replace 的旧文本不能为空");
    }
    std::size_t position = 0;
    while ((position = text.find(old, position)) != std::string::npos)
    {
        text.replace(position, old.size(), replacement);
        position += replacement.size();
    }
    return text;
}

tx_array tx_fn_split(std::string text, std::string separator)
{
    if (separator.empty())
    {
        throw std::runtime_error("split 的分隔符不能为空");
    }
    tx_array parts;
    std::size_t start = 0;
    while (true)
    {
        const auto position = text.find(separator, start);
        if (position == std::string::npos)
        {
            parts.emplace_back(text.substr(start));
            return parts;
        }
        parts.emplace_back(text.substr(start, position - start));
        start = position + separator.size();
    }
}

std::string tx_fn_join(tx_array parts, std::string separator)
{
    std::string result;
    for (std::size_t index = 0; index < parts.size(); ++index)
    {
        if (parts[index].type() != typeid(std::string))
        {
            throw std::runtime_error("join 的数组只能包含 str");
        }
        if (index != 0)
        {
            result += separator;
        }
        result += std::any_cast<const std::string&>(parts[index]);
    }
    return result;
}

std::string tx_fn_trim(std::string text)
{
    std::size_t first = 0;
    std::size_t last = text.size();
    while (first < last && is_ascii_space(text[first]))
    {
        ++first;
    }
    while (last > first && is_ascii_space(text[last - 1]))
    {
        --last;
    }
    return text.substr(first, last - first);
}

std::string tx_fn_lower(std::string text)
{
    for (char& value : text)
    {
        if (value >= 'A' && value <= 'Z')
        {
            value = static_cast<char>(value - 'A' + 'a');
        }
    }
    return text;
}

std::string tx_fn_upper(std::string text)
{
    for (char& value : text)
    {
        if (value >= 'a' && value <= 'z')
        {
            value = static_cast<char>(value - 'a' + 'A');
        }
    }
    return text;
}

} // namespace tx_generated
