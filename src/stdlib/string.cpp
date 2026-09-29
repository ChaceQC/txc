#include "stdlib/stdlib.hpp"
#include "common/utf8.hpp"

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

bool is_ascii_space(char value)
{
    return value == ' ' || value == '\t' || value == '\n' ||
           value == '\r' || value == '\v' || value == '\f';
}

} // namespace

tx_int tx_len(std::string_view text)
{
    if (text.size() < 32)
    {
        tx_int count = 0;
        for (std::size_t offset = 0; offset < text.size(); ++count)
        {
            offset = next_utf8(text, offset);
        }
        return count;
    }
    const auto scanned = tx::scan_utf8(text);
    if (!scanned.valid)
    {
        throw std::runtime_error("字符串包含无效 UTF-8");
    }
    if (scanned.length > static_cast<std::size_t>(std::numeric_limits<tx_int>::max()))
    {
        throw std::overflow_error("字符串长度超出 int 范围");
    }
    return static_cast<tx_int>(scanned.length);
}

tx_int tx_len(const std::string& text)
{
    return tx_len(std::string_view(text));
}

bool tx_fn_contains(std::string_view text, std::string_view part)
{
    return text.find(part) != std::string::npos;
}

bool tx_fn_starts_with(std::string_view text, std::string_view prefix)
{
    return text.starts_with(prefix);
}

bool tx_fn_ends_with(std::string_view text, std::string_view suffix)
{
    return text.ends_with(suffix);
}

tx_int tx_fn_find(std::string_view text, std::string_view part)
{
    (void)tx_len(text);
    const auto position = text.find(part);
    if (position == std::string::npos)
    {
        return -1;
    }
    return tx_len(text.substr(0, position));
}

std::string tx_fn_slice(std::string_view text, tx_int start, tx_int end)
{
    tx_int length = 0;
    std::size_t first = 0;
    std::size_t last = 0;
    // 一次验证完整 UTF-8，同时记录两个字符边界，保持原错误优先级。
    for (std::size_t offset = 0; offset < text.size();)
    {
        if (length == start)
        {
            first = offset;
        }
        if (length == end)
        {
            last = offset;
        }
        offset = next_utf8(text, offset);
        if (length == std::numeric_limits<tx_int>::max())
        {
            throw std::overflow_error("字符串长度超出 int 范围");
        }
        ++length;
    }
    if (start < 0 || end < start || end > length)
    {
        throw std::out_of_range("字符串切片范围无效");
    }
    if (start == length)
    {
        first = text.size();
    }
    if (end == length)
    {
        last = text.size();
    }
    return std::string(text.substr(first, last - first));
}

std::string tx_fn_replace(std::string_view text, std::string_view old,
                          std::string_view replacement)
{
    if (old.empty())
    {
        throw std::runtime_error("replace 的旧文本不能为空");
    }
    std::string result;
    result.reserve(text.size());
    std::size_t position = 0;
    while (true)
    {
        const auto found = text.find(old, position);
        if (found == std::string_view::npos)
        {
            result.append(text.substr(position));
            return result;
        }
        result.append(text.substr(position, found - position));
        result.append(replacement);
        position = found + old.size();
    }
}

tx_array tx_fn_split(std::string_view text, std::string_view separator)
{
    if (separator.empty())
    {
        throw std::runtime_error("split 的分隔符不能为空");
    }
    tx_array parts;
    std::size_t expected_parts = 1;
    std::size_t next = 0;
    while ((next = text.find(separator, next)) != std::string_view::npos)
    {
        ++expected_parts;
        next += separator.size();
    }
    parts.reserve(expected_parts);
    std::size_t start = 0;
    while (true)
    {
        const auto position = text.find(separator, start);
        if (position == std::string::npos)
        {
            parts.emplace_back(std::string(text.substr(start)));
            return parts;
        }
        parts.emplace_back(std::string(text.substr(start, position - start)));
        start = position + separator.size();
    }
}

std::string tx_fn_join(const tx_array& parts, std::string_view separator)
{
    std::size_t total = parts.size() == 0 ? 0 :
        separator.size() * (parts.size() - 1);
    for (const auto& item : parts)
    {
        if (item.type() != typeid(std::string))
        {
            throw std::runtime_error("join 的数组只能包含 str");
        }
        total += std::any_cast<const std::string&>(item).size();
    }
    std::string result;
    result.reserve(total);
    for (std::size_t index = 0; index < parts.size(); ++index)
    {
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
