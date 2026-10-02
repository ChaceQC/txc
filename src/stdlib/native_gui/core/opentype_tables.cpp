#include "stdlib/native_gui/core/opentype_internal.hpp"
#include <bit>

namespace tx::ui::ot
{
font_reader subtable(font_reader data, std::size_t offset)
{
    if (!offset || offset > data.size())
    {
        throw std::invalid_argument("OpenType 表偏移非法");
    }
    return font_reader(data.slice(offset, data.size() - offset));
}

int coverage(font_reader data, std::uint16_t glyph)
{
    const auto format = data.u16(0), count = data.u16(2);
    if (format != 1 && format != 2)
    {
        throw std::invalid_argument("OpenType Coverage 格式非法");
    }
    const unsigned stride = format == 1 ? 2 : 6;
    data.slice(4, std::size_t(count) * stride);
    unsigned low = 0, high = count;
    while (low < high)
    {
        const auto mid = low + (high - low) / 2, offset = 4 + mid * stride;
        const auto first = data.u16(offset), last = format == 1 ? first : data.u16(offset + 2);
        if (first > last)
        {
            throw std::invalid_argument("OpenType Coverage 区间倒置");
        }
        if (glyph < first)
        {
            high = mid;
        }
        else if (glyph > last)
        {
            low = mid + 1;
        }
        else
        {
            return format == 1 ? int(mid) : int(data.u16(offset + 4)) + glyph - first;
        }
    }
    return -1;
}

unsigned class_of(font_reader data, std::uint16_t glyph)
{
    if (!data.size())
    {
        return 0;
    }
    const auto format = data.u16(0);
    if (format == 1)
    {
        const auto start = data.u16(2), count = data.u16(4);
        data.slice(6, std::size_t(count) * 2);
        return glyph >= start && unsigned(glyph - start) < count ? data.u16(6 + (glyph - start) * 2) : 0;
    }
    if (format != 2)
    {
        throw std::invalid_argument("OpenType ClassDef 格式非法");
    }
    const auto count = data.u16(2);
    data.slice(4, std::size_t(count) * 6);
    unsigned low = 0, high = count;
    while (low < high)
    {
        const auto mid = low + (high - low) / 2, offset = 4 + mid * 6;
        const auto first = data.u16(offset), last = data.u16(offset + 2);
        if (first > last)
        {
            throw std::invalid_argument("OpenType ClassDef 区间倒置");
        }
        if (glyph < first)
        {
            high = mid;
        }
        else if (glyph > last)
        {
            low = mid + 1;
        }
        else
        {
            return data.u16(offset + 4);
        }
    }
    return 0;
}

unsigned processor::glyph_class(const shaped_glyph& glyph) const
{
    if (gdef_.size() && gdef_.u16(4))
    {
        return class_of(subtable(gdef_, gdef_.u16(4)), glyph.glyph);
    }
    return glyph.glyph_class;
}

bool processor::ignored(const lookup& entry, std::size_t position) const
{
    const auto& glyph = glyphs[position];
    const auto kind = glyph_class(glyph);
    if ((kind == 1 && (entry.flags & 2)) || (kind == 2 && (entry.flags & 4)) ||
        (kind == 3 && (entry.flags & 8)))
    {
        return true;
    }
    if (kind != 3)
    {
        return false;
    }
    if (entry.flags & 16)
    {
        if (!gdef_.size() || gdef_.u32(0) < 0x00010002 || !gdef_.u16(12))
        {
            throw std::invalid_argument("Lookup 引用了缺失的 GDEF MarkGlyphSetsDef");
        }
        const auto sets = subtable(gdef_, gdef_.u16(12));
        if (sets.u16(0) != 1 || entry.mark_set >= sets.u16(2))
        {
            throw std::invalid_argument("GDEF 标记过滤集合越界");
        }
        return coverage(subtable(sets, sets.u32(4 + entry.mark_set * 4)), glyph.glyph) < 0;
    }
    if (entry.flags & 0xff00)
    {
        return !gdef_.size() || !gdef_.u16(10) ||
            class_of(subtable(gdef_, gdef_.u16(10)), glyph.glyph) != entry.flags >> 8;
    }
    return false;
}

std::optional<std::size_t> processor::next(const lookup& entry, std::size_t position, int direction)
{
    while (direction > 0 ? position + 1 < glyphs.size() : position > 0)
    {
        spend();
        position = direction > 0 ? position + 1 : position - 1;
        if (!ignored(entry, position))
        {
            return position;
        }
    }
    return std::nullopt;
}

unsigned value_size(unsigned format)
{
    if (format & 0xff00)
    {
        throw std::invalid_argument("GPOS ValueFormat 保留位非法");
    }
    return std::popcount(format) * 2;
}
}
