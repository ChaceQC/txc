#include "stdlib/native_gui/core/font.hpp"

#include <stdexcept>

namespace tx::ui
{
void font_face::choose_cmap()
{
    const auto cmap = table("cmap");
    const auto count = cmap.u16(2);
    cmap.slice(4, std::size_t(count) * 8);
    int best = 0;
    for (unsigned index = 0; index < count; ++index)
    {
        const auto platform = cmap.u16(4 + index * 8), encoding = cmap.u16(6 + index * 8);
        if (platform != 0 && !(platform == 3 && (encoding == 1 || encoding == 10)))
        {
            continue;
        }
        const auto offset = cmap.u32(8 + index * 8);
        const auto format = cmap.u16(offset);
        const int score = format == 12 ? 2 : format == 4 ? 1 : 0;
        if (score > best)
        {
            const auto length = format == 12 ? cmap.u32(offset + 4) : cmap.u16(offset + 2);
            cmap.slice(offset, length);
            cmap_offset_ = offset;
            cmap_format_ = format;
            best = score;
        }
    }
    if (!best)
    {
        throw std::invalid_argument("字体缺少 Unicode cmap format 4/12");
    }
    const auto length = cmap_format_ == 12 ? cmap.u32(cmap_offset_ + 4) : cmap.u16(cmap_offset_ + 2);
    const font_reader selected(cmap.slice(cmap_offset_, length));
    std::uint32_t previous = 0;
    if (cmap_format_ == 12)
    {
        const auto groups = selected.u32(12);
        selected.slice(16, std::size_t(groups) * 12);
        for (std::size_t i = 0; i < groups; ++i)
        {
            const auto start = selected.u32(16 + i * 12), end = selected.u32(20 + i * 12);
            const auto glyph = selected.u32(24 + i * 12);
            if (start > end || end > 0x10ffff || (i && start <= previous) || std::uint64_t(glyph) + end - start >= glyph_count_)
            {
                throw std::invalid_argument("cmap format 12 区间无序或映射越界");
            }
            previous = end;
        }
    }
    else
    {
        const unsigned count = selected.u16(6) / 2;
        if (!count || selected.u16(6) % 2)
        {
            throw std::invalid_argument("cmap format 4 段数量非法");
        }
        selected.slice(14, count * 8 + 2);
        for (unsigned i = 0; i < count; ++i)
        {
            const auto end = selected.u16(14 + i * 2), start = selected.u16(16 + count * 2 + i * 2);
            const auto position = 16 + count * 6 + i * 2;
            const auto range = selected.u16(position);
            if (start > end || (i && start <= previous) || range % 2)
            {
                throw std::invalid_argument("cmap format 4 区间无序或偏移未对齐");
            }
            if (range)
            {
                selected.slice(position + range, (end - start + 1) * 2);
            }
            previous = end;
        }
    }
}

std::uint16_t font_face::glyph(char32_t scalar) const
{
    if (scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff))
    {
        throw std::invalid_argument("字符不是 Unicode 标量");
    }
    const auto table_data = table("cmap");
    const auto length = cmap_format_ == 12 ? table_data.u32(cmap_offset_ + 4) : table_data.u16(cmap_offset_ + 2);
    const font_reader data(table_data.slice(cmap_offset_, length));
    std::uint32_t result = 0;
    if (cmap_format_ == 12)
    {
        const auto count = data.u32(12);
        if (count > (data.size() - 16) / 12)
        {
            throw std::invalid_argument("cmap format 12 分组数量越界");
        }
        unsigned low = 0, high = count;
        while (low < high)
        {
            const auto middle = low + (high - low) / 2;
            const auto offset = 16 + std::size_t(middle) * 12;
            if (scalar < data.u32(offset))
            {
                high = middle;
            }
            else if (scalar > data.u32(offset + 4))
            {
                low = middle + 1;
            }
            else
            {
                const std::uint64_t mapped = std::uint64_t(data.u32(offset + 8)) + scalar - data.u32(offset);
                if (mapped >= glyph_count_)
                {
                    throw std::invalid_argument("cmap 映射的字形编号越界");
                }
                result = static_cast<std::uint32_t>(mapped);
                break;
            }
        }
    }
    else if (scalar <= 0xffff)
    {
        const unsigned count = data.u16(6) / 2;
        if (!count || data.u16(6) % 2)
        {
            throw std::invalid_argument("cmap format 4 段数量非法");
        }
        data.slice(14, count * 8 + 2);
        unsigned index = 0, upper = count;
        while (index < upper)
        {
            const auto middle = index + (upper - index) / 2;
            if (scalar > data.u16(14 + middle * 2))
            {
                index = middle + 1;
            }
            else
            {
                upper = middle;
            }
        }
        if (index < count && scalar >= data.u16(16 + count * 2 + index * 2))
        {
            const auto start = data.u16(16 + count * 2 + index * 2);
            const auto delta = data.i16(16 + count * 4 + index * 2);
            const auto range_position = 16 + count * 6 + index * 2;
            const auto range = data.u16(range_position);
            result = range ? data.u16(range_position + range + (scalar - start) * 2) : scalar;
            if (!range || result)
            {
                result = (result + delta) & 0xffff;
            }
        }
    }
    if (result >= glyph_count_)
    {
        throw std::invalid_argument("cmap 映射的字形编号越界");
    }
    return static_cast<std::uint16_t>(result);
}

double font_face::advance(std::uint16_t glyph, double size) const
{
    if (glyph >= glyph_count_ || !std::isfinite(size) || size <= 0 || size > 4096)
    {
        throw std::invalid_argument("字形编号或字号非法");
    }
    return table("hmtx").u16(std::min<unsigned>(glyph, metrics_count_ - 1) * 4) * size / units_;
}

double font_face::ascender(double size) const
{
    advance(0, size);
    return ascender_ * size / units_;
}

double font_face::line_height(double size) const
{
    advance(0, size);
    return std::max<double>(units_, int(ascender_) - descender_ + line_gap_) * size / units_;
}
}
