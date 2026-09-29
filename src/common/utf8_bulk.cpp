#include "common/utf8.hpp"

#include <cstdint>

namespace tx::detail
{
namespace
{

bool between(std::uint8_t value, std::uint8_t first, std::uint8_t last) noexcept
{
    return static_cast<std::uint8_t>(value - first) <= last - first;
}

std::uint8_t invalid_byte(std::uint8_t current, std::uint8_t previous,
    std::uint8_t second_previous, std::uint8_t third_previous) noexcept
{
    const bool continuation = between(current, 0x80, 0xbf);
    // 每个续字节必须有距其 1/2/3 字节的合法前导字节；反向约束也必须成立。
    const bool expected = between(previous, 0xc2, 0xf4) |
        between(second_previous, 0xe0, 0xf4) | between(third_previous, 0xf0, 0xf4);
    const bool forbidden = (current == 0xc0) | (current == 0xc1) | (current >= 0xf5);
    const bool restricted = ((previous == 0xe0) & (current < 0xa0)) |
        ((previous == 0xed) & (current >= 0xa0)) |
        ((previous == 0xf0) & (current < 0x90)) |
        ((previous == 0xf4) & (current >= 0x90));
    return (continuation != expected) | forbidden | restricted;
}

} // namespace

// 调用者保证至少有 32 字节；所有相邻读取和尾部哨兵均不访问视图外内存。
utf8_scan_result scan_utf8_bulk(std::string_view text) noexcept
{
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(text.data());
    std::size_t length = 0;
    std::uint8_t invalid = invalid_byte(bytes[0], 0, 0, 0) |
        invalid_byte(bytes[1], bytes[0], 0, 0) |
        invalid_byte(bytes[2], bytes[1], bytes[0], 0);
    for (std::size_t index = 0; index < 3; ++index)
    {
        length += !between(bytes[index], 0x80, 0xbf);
    }
    // 无循环内分支及循环携带的解码状态，普通编译器可向量化。
    for (std::size_t index = 3; index < text.size(); ++index)
    {
        invalid |= invalid_byte(bytes[index], bytes[index - 1], bytes[index - 2], bytes[index - 3]);
        length += !between(bytes[index], 0x80, 0xbf);
    }
    invalid |= invalid_byte(0, bytes[text.size() - 1], bytes[text.size() - 2], bytes[text.size() - 3]);
    return {invalid == 0, length};
}

} // namespace tx::detail
