#include "stdlib/native_gui/core/opentype_internal.hpp"
#include <algorithm>

namespace tx::ui::ot
{
double processor::device(font_reader data) const
{
    const auto start = data.u16(0), end = data.u16(2), format = data.u16(4);
    // 本轮固定默认字体实例；VariationIndex 的非默认坐标不在支持范围。
    if (format == 0x8000)
    {
        return 0;
    }
    if (format < 1 || format > 3 || end < start)
    {
        throw std::invalid_argument("OpenType Device 格式非法");
    }
    const unsigned bits = 1u << format, per_word = 16 / bits;
    data.slice(6, ((end - start + per_word) / per_word) * 2);
    // 分数像素字号不对应整像素 ppem 的 Device 项。
    const auto ppem = unsigned(std::round(size));
    if (std::abs(size - ppem) > 0.000001 || ppem < start || ppem > end)
    {
        return 0;
    }
    const unsigned at = ppem - start, shift = 16 - bits * (at % per_word + 1);
    const auto value = (data.u16(6 + (at / per_word) * 2) >> shift) & ((1u << bits) - 1);
    return value & (1u << (bits - 1)) ? int(value) - int(1u << bits) : int(value);
}

std::optional<point> processor::anchor(font_reader parent, unsigned offset) const
{
    if (!offset)
    {
        return std::nullopt;
    }
    const auto data = subtable(parent, offset);
    const auto format = data.u16(0);
    point result{data.i16(2) * scale, -data.i16(4) * scale};
    if (format == 2)
    {
        // 未启用 hinting 时，规范允许采用未拟合轮廓的设计坐标。
        data.u16(6);
    }
    else if (format == 3)
    {
        if (data.u16(6))
        {
            result.x += device(subtable(data, data.u16(6)));
        }
        if (data.u16(8))
        {
            result.y -= device(subtable(data, data.u16(8)));
        }
    }
    else if (format != 1)
    {
        throw std::invalid_argument("GPOS Anchor 格式非法");
    }
    return result;
}

void processor::value(font_reader data, std::size_t offset, unsigned format, shaped_glyph& glyph) const
{
    data.slice(offset, value_size(format));
    double values[4]{};
    for (unsigned i = 0; i < 8; ++i)
    {
        if (!(format & (1u << i)))
        {
            continue;
        }
        if (i < 4)
        {
            values[i] += data.i16(offset) * scale;
        }
        else if (data.u16(offset))
        {
            values[i - 4] += device(subtable(data, data.u16(offset)));
        }
        offset += 2;
    }
    glyph.x += values[0];
    glyph.y -= values[1];
    glyph.advance += values[2];
}

double processor::origin(std::size_t from, std::size_t to) const
{
    double result = 0;
    if (from > to)
    {
        return -origin(to, from);
    }
    for (auto i = from; i < to; ++i)
    {
        result += options.right_to_left ? -glyphs[i + 1].advance : glyphs[i].advance;
    }
    return result;
}

namespace
{
application single(processor& engine, font_reader data, std::size_t position, int covered)
{
    const auto format = data.u16(0), value_format = data.u16(4);
    if (format == 1)
    {
        engine.value(data, 6, value_format, engine.glyphs[position]);
    }
    else if (format == 2 && covered < data.u16(6))
    {
        engine.value(data, 8 + std::size_t(covered) * value_size(value_format), value_format, engine.glyphs[position]);
    }
    else
    {
        throw std::invalid_argument("GPOS Single 格式或覆盖索引非法");
    }
    return {true, position + 1};
}

application pair(processor& engine, const lookup& entry, font_reader data, std::size_t position, int covered)
{
    const auto second = engine.next(entry, position);
    if (!second)
    {
        return {};
    }
    const auto format = data.u16(0), first_format = data.u16(4), second_format = data.u16(6);
    const auto first_size = value_size(first_format), second_size = value_size(second_format);
    auto values = data;
    std::size_t offset = 0;
    if (format == 1)
    {
        if (covered >= data.u16(8))
        {
            throw std::invalid_argument("GPOS Pair 覆盖索引越界");
        }
        const auto set_offset = data.u16(10 + covered * 2);
        const auto set = subtable(data, set_offset);
        const auto count = set.u16(0);
        const auto stride = 2 + first_size + second_size;
        set.slice(2, std::size_t(count) * stride);
        unsigned low = 0, high = count;
        const auto wanted = engine.glyphs[*second].glyph;
        while (low < high)
        {
            const auto mid = low + (high - low) / 2;
            if (set.u16(2 + mid * stride) < wanted)
            {
                low = mid + 1;
            }
            else
            {
                high = mid;
            }
        }
        if (low == count || set.u16(2 + low * stride) != wanted)
        {
            return {};
        }
        // PairSet 内的 Device 偏移相对于 PairSet，不能误作 PairPos 偏移。
        values = set;
        offset = 4 + low * stride;
    }
    else if (format == 2)
    {
        const auto first = class_of(subtable(data, data.u16(8)), engine.glyphs[position].glyph);
        const auto next = class_of(subtable(data, data.u16(10)), engine.glyphs[*second].glyph);
        const auto first_count = data.u16(12), second_count = data.u16(14);
        if (first >= first_count || next >= second_count)
        {
            throw std::invalid_argument("GPOS Pair 类别索引越界");
        }
        data.slice(16, std::size_t(first_count) * second_count * (first_size + second_size));
        offset = 16 + (std::size_t(first) * second_count + next) * (first_size + second_size);
    }
    else
    {
        throw std::invalid_argument("GPOS Pair 格式非法");
    }
    engine.value(values, offset, first_format, engine.glyphs[position]);
    engine.value(values, offset + first_size, second_format, engine.glyphs[*second]);
    return {true, second_format ? *second + 1 : *second};
}
}

application processor::positioning(const lookup& entry, font_reader data, unsigned type,
    std::size_t position, unsigned depth)
{
    if (type == 7 || type == 8)
    {
        return context(entry, data, type == 8, position, depth);
    }
    const auto covered = coverage(subtable(data, data.u16(2)), glyphs[position].glyph);
    if (covered < 0)
    {
        return {};
    }
    if (type == 1)
    {
        return single(*this, data, position, covered);
    }
    if (type == 2)
    {
        return pair(*this, entry, data, position, covered);
    }
    if (type >= 3 && type <= 6)
    {
        return attach(entry, data, type, position);
    }
    throw std::invalid_argument("GPOS Lookup 类型非法");
}
}
