#include "stdlib/cbor_internal.hpp"
#include "stdlib/error.hpp"
#include "stdlib/json_utf8.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <utility>

namespace tx_generated
{
namespace
{

[[noreturn]] void output_error(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::runtime, code, message});
}

std::uint16_t half_bits(float value)
{
    const auto bits = std::bit_cast<std::uint32_t>(value);
    const auto sign = static_cast<std::uint16_t>((bits >> 16) & 0x8000);
    const auto exponent = static_cast<int>((bits >> 23) & 0xff) - 127;
    const auto fraction = bits & 0x7fffff;
    if (((bits >> 23) & 0xff) == 255)
    {
        return static_cast<std::uint16_t>(sign | 0x7c00 | (fraction ? 0x0200 : 0));
    }
    if (exponent > 15)
    {
        return static_cast<std::uint16_t>(sign | 0x7c00);
    }
    if (exponent < -25)
    {
        return sign;
    }
    if (exponent >= -14)
    {
        const auto rounded = fraction + 0x0fff + ((fraction >> 13) & 1);
        auto half_exponent = exponent + 15;
        auto half_fraction = rounded >> 13;
        if (half_fraction == 1024)
        {
            ++half_exponent;
            half_fraction = 0;
        }
        return static_cast<std::uint16_t>(sign | (half_exponent << 10) |
            half_fraction);
    }
    const auto shift = static_cast<unsigned>(-exponent - 1);
    const auto significand = fraction | 0x800000;
    auto mantissa = significand >> shift;
    const auto remainder = significand & ((1u << shift) - 1);
    const auto halfway = 1u << (shift - 1);
    if (remainder > halfway || (remainder == halfway && (mantissa & 1)))
    {
        ++mantissa;
    }
    return static_cast<std::uint16_t>(sign | mantissa);
}

bool same_float(double left, double right)
{
    return std::bit_cast<std::uint64_t>(left) == std::bit_cast<std::uint64_t>(right);
}

} // namespace

void cbor_check_limits(const cbor_limits& limits)
{
    if (limits.max_bytes <= 0 || limits.max_bytes > 1099511627776LL ||
        limits.max_value_bytes <= 0 || limits.max_value_bytes > 67108864 ||
        limits.max_depth <= 0 || limits.max_depth > 128 ||
        limits.max_items <= 0 || limits.max_items > 1000000)
    {
        output_error("invalid_argument", "CBOR 字节、单值、深度或元素限额无效");
    }
}

bool cbor_valid_utf8(std::string_view value)
{
    for (std::size_t offset = 0; offset < value.size();)
    {
        const auto width = json_detail::utf8_width(value, offset);
        if (width == 0)
        {
            return false;
        }
        offset += width;
    }
    return true;
}

bool cbor_key_less(std::string_view left, std::string_view right)
{
    if (left.size() != right.size())
    {
        return left.size() < right.size();
    }
    for (std::size_t index = 0; index < left.size(); ++index)
    {
        const auto a = static_cast<unsigned char>(left[index]);
        const auto b = static_cast<unsigned char>(right[index]);
        if (a != b)
        {
            return a < b;
        }
    }
    return false;
}

std::string cbor_head_bytes(std::uint8_t major, std::uint64_t argument)
{
    std::string result;
    if (argument < 24)
    {
        result.push_back(static_cast<char>((major << 5) | argument));
        return result;
    }
    const auto width = argument <= UINT8_MAX ? 1U : argument <= UINT16_MAX ? 2U
        : argument <= UINT32_MAX ? 4U : 8U;
    const auto additional = width == 1 ? 24U : width == 2 ? 25U : width == 4 ? 26U : 27U;
    result.push_back(static_cast<char>((major << 5) | additional));
    for (unsigned index = width; index > 0; --index)
    {
        result.push_back(static_cast<char>(argument >> ((index - 1) * 8)));
    }
    return result;
}

double cbor_half_value(std::uint16_t bits)
{
    const auto exponent = (bits >> 10) & 31;
    const auto fraction = bits & 1023;
    double value = exponent == 0 ? std::ldexp(static_cast<double>(fraction), -24)
        : exponent == 31 ? (fraction ? std::numeric_limits<double>::quiet_NaN()
                                     : std::numeric_limits<double>::infinity())
        : std::ldexp(static_cast<double>(fraction + 1024), exponent - 25);
    return bits & 0x8000 ? -value : value;
}

std::string cbor_float_bytes(double value)
{
    if (std::isnan(value))
    {
        return std::string("\xf9\x7e\x00", 3);
    }
    const auto single = static_cast<float>(value);
    if (same_float(static_cast<double>(single), value))
    {
        const auto half = half_bits(single);
        if (same_float(cbor_half_value(half), value))
        {
            return {static_cast<char>(0xf9), static_cast<char>(half >> 8),
                    static_cast<char>(half)};
        }
        const auto bits = std::bit_cast<std::uint32_t>(single);
        return {static_cast<char>(0xfa), static_cast<char>(bits >> 24),
                static_cast<char>(bits >> 16), static_cast<char>(bits >> 8),
                static_cast<char>(bits)};
    }
    const auto bits = std::bit_cast<std::uint64_t>(value);
    std::string result(1, static_cast<char>(0xfb));
    for (int index = 7; index >= 0; --index)
    {
        result.push_back(static_cast<char>(bits >> (index * 8)));
    }
    return result;
}

cbor_output::cbor_output(format_sink sink, const cbor_limits& limits)
    : sink_(std::move(sink)), limits_(limits)
{
}

void cbor_output::append(std::string_view bytes)
{
    if (bytes.size() > static_cast<std::uint64_t>(limits_.max_bytes) - written_ ||
        (within_value_ && bytes.size() >
            static_cast<std::uint64_t>(limits_.max_value_bytes) -
                (written_ - value_start_)))
    {
        output_error("size_limit", "CBOR 输出超过配置的字节上限");
    }
    written_ += bytes.size();
    while (!bytes.empty())
    {
        const auto count = std::min<std::size_t>(4096 - buffer_.size(), bytes.size());
        buffer_.append(bytes.data(), count);
        bytes.remove_prefix(count);
        if (buffer_.size() == 4096)
        {
            flush();
        }
    }
}

void cbor_output::begin_value()
{
    value_start_ = written_;
    within_value_ = true;
}

void cbor_output::end_value()
{
    within_value_ = false;
}

void cbor_output::flush()
{
    if (!buffer_.empty())
    {
        sink_(buffer_);
        buffer_.clear();
    }
}

std::uint64_t cbor_output::written() const noexcept
{
    return written_;
}

} // namespace tx_generated
