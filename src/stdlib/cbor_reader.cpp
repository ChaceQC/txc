#include "stdlib/cbor_internal.hpp"

#include <bit>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace tx_generated
{
namespace
{

std::uint64_t read_integer(format_input& input, unsigned width)
{
    std::uint64_t result = 0;
    for (unsigned index = 0; index < width; ++index)
    {
        result = (result << 8) | static_cast<unsigned char>(input.get());
    }
    return result;
}

std::string read_blob(format_input& input, std::uint64_t count,
                      const cbor_limits& limits)
{
    if (count > static_cast<std::uint64_t>(limits.max_value_bytes))
    {
        input.fail("size_limit", "CBOR 字符串或字节串超过单值上限");
    }
    std::string result;
    result.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t index = 0; index < count; ++index)
    {
        result.push_back(input.get());
    }
    return result;
}

std::any parse_float(format_input& input, const cbor_head& head)
{
    if (head.additional != 25 && head.additional != 26 && head.additional != 27)
    {
        input.fail("type_mismatch", "CBOR 简单值不在支持范围内");
    }
    const auto width = head.additional == 25 ? 2U : head.additional == 26 ? 4U : 8U;
    const auto bits = head.argument;
    double value = 0;
    if (width == 2)
    {
        value = cbor_half_value(static_cast<std::uint16_t>(bits));
    }
    else if (width == 4)
    {
        value = static_cast<double>(std::bit_cast<float>(static_cast<std::uint32_t>(bits)));
    }
    else
    {
        value = std::bit_cast<double>(bits);
    }
    std::string original(1, static_cast<char>((7 << 5) | head.additional));
    for (unsigned index = width; index > 0; --index)
    {
        original.push_back(static_cast<char>(bits >> ((index - 1) * 8)));
    }
    if (original != cbor_float_bytes(value))
    {
        input.fail("non_canonical", "CBOR 浮点数不是最短规范表示");
    }
    return value;
}

std::any parse_map(format_input& input, const cbor_limits& limits,
                   std::size_t depth, std::uint64_t count)
{
    if (count > static_cast<std::uint64_t>(limits.max_items))
    {
        input.fail("size_limit", "CBOR 映射长度超过元素上限");
    }
    tx_dict result;
    std::string previous;
    for (std::uint64_t index = 0; index < count; ++index)
    {
        auto key = cbor_parse_value(input, limits, depth + 1);
        if (key.has_value() && key.type() != typeid(std::int64_t) &&
            key.type() != typeid(double) && key.type() != typeid(bool) &&
            key.type() != typeid(std::string))
        {
            input.fail("type_mismatch", "CBOR 映射键不能映射到 TX dict");
        }
        if (const auto* number = std::any_cast<double>(&key);
            number && !std::isfinite(*number))
        {
            input.fail("type_mismatch", "CBOR 映射键不能是 NaN 或无穷大");
        }
        const auto encoded = cbor_encode_key(key);
        if (result.find_value(key) != nullptr ||
            (!previous.empty() && previous == encoded))
        {
            input.fail("duplicate_key", "CBOR 映射包含重复键");
        }
        if (!previous.empty() && !cbor_key_less(previous, encoded))
        {
            input.fail("non_canonical", "CBOR 映射键未按规范顺序排列");
        }
        previous = encoded;
        auto value = cbor_parse_value(input, limits, depth + 1);
        (void)result.emplace_back(std::move(key), std::move(value));
    }
    return result;
}

} // namespace

cbor_head cbor_read_head(format_input& input)
{
    const auto first = static_cast<std::uint8_t>(input.get());
    cbor_head head{static_cast<std::uint8_t>(first >> 5),
                   static_cast<std::uint8_t>(first & 31), 0};
    if (head.additional < 24)
    {
        head.argument = head.additional;
    }
    else if (head.additional >= 24 && head.additional <= 27)
    {
        const auto width = 1U << (head.additional - 24);
        head.argument = read_integer(input, width);
    }
    else
    {
        input.fail("invalid_syntax", "CBOR 不接受无限长项或保留的头部");
    }
    if (head.major <= 5 &&
        ((head.additional == 24 && head.argument < 24) ||
         (head.additional == 25 && head.argument <= UINT8_MAX) ||
         (head.additional == 26 && head.argument <= UINT16_MAX) ||
         (head.additional == 27 && head.argument <= UINT32_MAX)))
    {
        input.fail("non_canonical", "CBOR 整数或长度头不是最短表示");
    }
    return head;
}

std::any cbor_parse_value(format_input& input, const cbor_limits& limits,
                          std::size_t depth)
{
    if (depth > static_cast<std::size_t>(limits.max_depth))
    {
        input.fail("depth_limit", "CBOR 嵌套超过深度上限");
    }
    const auto head = cbor_read_head(input);
    switch (head.major)
    {
    case 0:
        if (head.argument > INT64_MAX)
        {
            input.fail("out_of_range", "CBOR 整数超出 TX int 范围");
        }
        return static_cast<std::int64_t>(head.argument);
    case 1:
        if (head.argument > INT64_MAX)
        {
            input.fail("out_of_range", "CBOR 负整数超出 TX int 范围");
        }
        return static_cast<std::int64_t>(-1 - static_cast<std::int64_t>(head.argument));
    case 2:
    {
        const auto bytes = read_blob(input, head.argument, limits);
        return make_bytes({bytes.begin(), bytes.end()});
    }
    case 3:
    {
        auto value = read_blob(input, head.argument, limits);
        if (!cbor_valid_utf8(value))
        {
            input.fail("invalid_utf8", "CBOR 文本包含无效 UTF-8");
        }
        return value;
    }
    case 4:
    {
        if (head.argument > static_cast<std::uint64_t>(limits.max_items))
        {
            input.fail("size_limit", "CBOR 数组长度超过元素上限");
        }
        tx_array result;
        for (std::uint64_t index = 0; index < head.argument; ++index)
        {
            result.push_back(cbor_parse_value(input, limits, depth + 1));
        }
        return result;
    }
    case 5: return parse_map(input, limits, depth, head.argument);
    case 7:
        if (head.additional == 20)
        {
            return false;
        }
        if (head.additional == 21)
        {
            return true;
        }
        if (head.additional == 22)
        {
            return std::any{};
        }
        return parse_float(input, head);
    default:
        input.fail("type_mismatch", "CBOR 标签或该值类型不在支持范围内");
    }
}

void cbor_require_end(format_input& input)
{
    if (input.peek() >= 0)
    {
        input.fail("invalid_syntax", "CBOR 根值后存在多余字节");
    }
}

} // namespace tx_generated
