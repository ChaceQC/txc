#include "common/utf8.hpp"

#include <cstdint>
#include <cstring>

namespace tx
{
namespace detail
{

utf8_scan_result scan_utf8_bulk(std::string_view text) noexcept;

} // namespace detail

utf8_scan_result scan_utf8(std::string_view text) noexcept
{
    std::size_t ascii = 0;
    // memcpy 支持非对齐读取且不违反别名规则；每次读取前验证剩余长度。
    while (text.size() - ascii >= sizeof(std::uint64_t))
    {
        std::uint64_t word = 0;
        std::memcpy(&word, text.data() + ascii, sizeof(word));
        if ((word & UINT64_C(0x8080808080808080)) != 0)
        {
            break;
        }
        ascii += sizeof(word);
    }
    while (ascii < text.size() && static_cast<unsigned char>(text[ascii]) < 0x80)
    {
        ++ascii;
    }
    text.remove_prefix(ascii);
    std::size_t length = ascii;
    if (text.size() < 32)
    {
        for (std::size_t offset = 0; offset < text.size();)
        {
            const auto width = utf8_width(text, offset);
            if (width == 0)
            {
                return {false, 0};
            }
            offset += width;
            ++length;
        }
        return {true, length};
    }
    // 独立编译批量核，避免短路径也保存整组 SIMD 寄存器。
    const auto remaining = detail::scan_utf8_bulk(text);
    return {remaining.valid, ascii + remaining.length};
}

} // namespace tx
