#include "stdlib/bytes.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace tx_generated
{
namespace
{

constexpr char hex_digits[] = "0123456789abcdef";
constexpr char base64_digits[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void require_size(std::size_t size)
{
    if (size > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()))
    {
        throw runtime_failure({tx::error_kind::runtime, "size_limit",
                               "字节值长度超出 int 范围"});
    }
}

int hex_digit(char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f')
    {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F')
    {
        return character - 'A' + 10;
    }
    return -1;
}

int base64_digit(char character)
{
    const auto found = std::string_view(base64_digits).find(character);
    return found == std::string_view::npos ? -1 : static_cast<int>(found);
}

[[noreturn]] void invalid_base64()
{
    throw runtime_failure({tx::error_kind::parse, "invalid_base64",
                           "Base64 文本格式无效"});
}

} // namespace

byte_value make_bytes(std::vector<std::uint8_t> data)
{
    require_size(data.size());
    return std::make_shared<const std::vector<std::uint8_t>>(std::move(data));
}

const byte_value& bytes_of(const std::any& value)
{
    const auto* result = std::any_cast<byte_value>(&value);
    if (!result || !*result)
    {
        throw std::runtime_error("需要 bytes 值");
    }
    return *result;
}

std::int64_t bytes_length(const byte_value& value)
{
    require_size(value->size());
    return static_cast<std::int64_t>(value->size());
}

std::int64_t bytes_at(const byte_value& value, std::int64_t index)
{
    if (index < 0 || static_cast<std::uint64_t>(index) >= value->size())
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_argument",
                               "bytes 索引越界"});
    }
    return (*value)[static_cast<std::size_t>(index)];
}

byte_value bytes_from_vector(const int_vector& values)
{
    const auto& source = values.data().values;
    require_size(source.size());
    std::vector<std::uint8_t> result;
    result.reserve(source.size());
    for (const auto item : source)
    {
        if (item < 0 || item > 255)
        {
            throw runtime_failure({tx::error_kind::runtime, "invalid_argument",
                                   "字节元素必须在 0 到 255 之间"});
        }
        result.push_back(static_cast<std::uint8_t>(item));
    }
    return make_bytes(std::move(result));
}

int_vector bytes_to_vector(const byte_value& value)
{
    int_vector result;
    auto& data = result.data();
    data.values.reserve(value->size());
    for (const auto item : *value)
    {
        data.values.push_back(item);
    }
    data.refresh();
    return result;
}

byte_value bytes_concat(const byte_value& left, const byte_value& right)
{
    if (right->size() > static_cast<std::size_t>(
            std::numeric_limits<std::int64_t>::max()) - left->size())
    {
        throw runtime_failure({tx::error_kind::runtime, "size_limit",
                               "拼接后的字节值过长"});
    }
    std::vector<std::uint8_t> result;
    result.reserve(left->size() + right->size());
    result.insert(result.end(), left->begin(), left->end());
    result.insert(result.end(), right->begin(), right->end());
    return make_bytes(std::move(result));
}

byte_value bytes_slice(const byte_value& value, std::int64_t start,
                       std::int64_t end)
{
    if (start < 0 || end < start || static_cast<std::uint64_t>(end) > value->size())
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_argument",
                               "bytes 切片范围无效"});
    }
    return make_bytes({value->begin() + start, value->begin() + end});
}

std::string bytes_to_hex(const byte_value& value)
{
    if (value->size() > static_cast<std::size_t>(
            std::numeric_limits<std::int64_t>::max()) / 2)
    {
        throw runtime_failure({tx::error_kind::runtime, "size_limit",
                               "十六进制文本过长"});
    }
    std::string result;
    result.reserve(value->size() * 2);
    for (const auto item : *value)
    {
        result.push_back(hex_digits[item >> 4]);
        result.push_back(hex_digits[item & 15]);
    }
    return result;
}

byte_value bytes_from_hex(std::string_view text)
{
    if (text.size() % 2 != 0)
    {
        throw runtime_failure({tx::error_kind::parse, "invalid_hex",
                               "十六进制文本长度必须为偶数"});
    }
    require_size(text.size() / 2);
    std::vector<std::uint8_t> result;
    result.reserve(text.size() / 2);
    for (std::size_t index = 0; index < text.size(); index += 2)
    {
        const int high = hex_digit(text[index]);
        const int low = hex_digit(text[index + 1]);
        if (high < 0 || low < 0)
        {
            throw runtime_failure({tx::error_kind::parse, "invalid_hex",
                                   "十六进制文本包含无效字符"});
        }
        result.push_back(static_cast<std::uint8_t>((high << 4) | low));
    }
    return make_bytes(std::move(result));
}

std::string bytes_to_base64(const byte_value& value)
{
    if (value->size() > static_cast<std::size_t>(
            std::numeric_limits<std::int64_t>::max()) / 4 * 3)
    {
        throw runtime_failure({tx::error_kind::runtime, "size_limit",
                               "Base64 文本过长"});
    }
    std::string result;
    result.reserve((value->size() + 2) / 3 * 4);
    for (std::size_t index = 0; index < value->size(); index += 3)
    {
        const unsigned first = (*value)[index];
        const unsigned second = index + 1 < value->size() ? (*value)[index + 1] : 0;
        const unsigned third = index + 2 < value->size() ? (*value)[index + 2] : 0;
        result.push_back(base64_digits[first >> 2]);
        result.push_back(base64_digits[((first & 3) << 4) | (second >> 4)]);
        result.push_back(index + 1 < value->size()
            ? base64_digits[((second & 15) << 2) | (third >> 6)] : '=');
        result.push_back(index + 2 < value->size()
            ? base64_digits[third & 63] : '=');
    }
    return result;
}

byte_value bytes_from_base64(std::string_view text)
{
    if (text.size() % 4 != 0)
    {
        invalid_base64();
    }
    require_size(text.size() / 4 * 3);
    std::vector<std::uint8_t> result;
    result.reserve(text.size() / 4 * 3);
    for (std::size_t index = 0; index < text.size(); index += 4)
    {
        const int first = base64_digit(text[index]);
        const int second = base64_digit(text[index + 1]);
        const bool last = index + 4 == text.size();
        const bool third_padding = text[index + 2] == '=';
        const bool fourth_padding = text[index + 3] == '=';
        const int third = third_padding ? 0 : base64_digit(text[index + 2]);
        const int fourth = fourth_padding ? 0 : base64_digit(text[index + 3]);
        if (first < 0 || second < 0 || third < 0 || fourth < 0 ||
            (!last && (third_padding || fourth_padding)) ||
            (third_padding && !fourth_padding) ||
            (third_padding && (second & 15) != 0) ||
            (fourth_padding && !third_padding && (third & 3) != 0))
        {
            invalid_base64();
        }
        result.push_back(static_cast<std::uint8_t>((first << 2) | (second >> 4)));
        if (!third_padding)
        {
            result.push_back(static_cast<std::uint8_t>((second << 4) | (third >> 2)));
        }
        if (!fourth_padding)
        {
            result.push_back(static_cast<std::uint8_t>((third << 6) | fourth));
        }
    }
    return make_bytes(std::move(result));
}

} // namespace tx_generated
