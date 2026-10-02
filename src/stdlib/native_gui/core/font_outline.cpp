#include "stdlib/native_gui/core/font.hpp"

#include <stdexcept>

namespace tx::ui
{
glyph_outline font_face::read_simple(font_reader data, unsigned count) const
{
    if (!count)
    {
        return {};
    }
    std::vector<unsigned> ends;
    for (unsigned index = 0; index < count; ++index)
    {
        const unsigned end = data.u16(10 + index * 2);
        if (!ends.empty() && end <= ends.back())
        {
            throw std::invalid_argument("字体轮廓端点未递增");
        }
        ends.push_back(end);
    }
    const auto total = ends.back() + 1;
    std::size_t cursor = 10 + count * 2;
    const auto instructions = data.u16(cursor);
    cursor += 2;
    data.slice(cursor, instructions);
    cursor += instructions;
    std::vector<std::uint8_t> flags;
    while (flags.size() < total)
    {
        const auto flag = data.u8(cursor++);
        const unsigned repeat = (flag & 8) ? data.u8(cursor++) + 1 : 1;
        if (repeat > total - flags.size())
        {
            throw std::invalid_argument("字体点标志重复数量越界");
        }
        flags.insert(flags.end(), repeat, flag);
    }
    std::vector<outline_point> points(total);
    for (const bool horizontal : {true, false})
    {
        int coordinate = 0;
        const unsigned short_mask = horizontal ? 2 : 4, same_mask = horizontal ? 16 : 32;
        for (unsigned index = 0; index < total; ++index)
        {
            if (flags[index] & short_mask)
            {
                const auto delta = data.u8(cursor++);
                coordinate += (flags[index] & same_mask) ? delta : -int(delta);
            }
            else if (!(flags[index] & same_mask))
            {
                coordinate += data.i16(cursor);
                cursor += 2;
            }
            if (coordinate < -32768 || coordinate > 32767)
            {
                throw std::invalid_argument("字体轮廓坐标超过 int16 范围");
            }
            (horizontal ? points[index].x : points[index].y) = coordinate;
            points[index].on_curve = (flags[index] & 1) != 0;
        }
    }
    glyph_outline result;
    unsigned first = 0;
    for (const auto end : ends)
    {
        result.emplace_back(points.begin() + first, points.begin() + end + 1);
        first = end + 1;
    }
    return result;
}

glyph_outline font_face::read_outline(std::uint16_t glyph, std::vector<std::uint16_t>& chain, std::size_t& budget) const
{
    if (!budget || glyph >= glyph_count_ || chain.size() >= 32 || std::find(chain.begin(), chain.end(), glyph) != chain.end())
    {
        throw std::invalid_argument("复合字形编号越界、递归成环、超过 32 层或解码预算耗尽");
    }
    --budget;
    const auto loca = table("loca"), glyf = table("glyf");
    const std::size_t start = long_locations_ ? loca.u32(glyph * 4) : loca.u16(glyph * 2) * 2;
    const std::size_t end = long_locations_ ? loca.u32((glyph + 1) * 4) : loca.u16((glyph + 1) * 2) * 2;
    if (start == end)
    {
        return {};
    }
    const font_reader data(glyf.slice(start, end - start));
    const auto count = data.i16(0);
    if (count >= 0)
    {
        return read_simple(data, static_cast<unsigned>(count));
    }
    chain.push_back(glyph);
    const auto result = read_compound(data, chain, budget);
    chain.pop_back();
    return result;
}

path font_face::outline(std::uint16_t glyph, double size, point baseline) const
{
    advance(glyph, size);
    const auto& source = cached_outline(glyph);
    path result;
    const double scale = size / units_;
    const auto transform = [&](outline_point value) -> point
    {
        return {baseline.x + value.x * scale, baseline.y - value.y * scale};
    };
    for (const auto& contour : source)
    {
        if (contour.empty())
        {
            continue;
        }
        const auto first = transform(contour.front()), last = transform(contour.back());
        const auto start = contour.front().on_curve ? first : contour.back().on_curve ? last :
            point{(first.x + last.x) / 2, (first.y + last.y) / 2};
        result.move_to(start);
        bool pending = false;
        point control;
        for (const auto& vertex : contour)
        {
            const auto position = transform(vertex);
            if (vertex.on_curve)
            {
                if (pending)
                {
                    result.quadratic_to(control, position);
                }
                else
                {
                    result.line_to(position);
                }
                pending = false;
            }
            else
            {
                if (pending)
                {
                    result.quadratic_to(control, {(control.x + position.x) / 2, (control.y + position.y) / 2});
                }
                control = position;
                pending = true;
            }
        }
        if (pending)
        {
            result.quadratic_to(control, start);
        }
    }
    return result;
}
}
