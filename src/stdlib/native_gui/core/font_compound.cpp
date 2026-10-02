#include "stdlib/native_gui/core/font.hpp"

#include <stdexcept>

namespace tx::ui
{
namespace
{
std::size_t point_count(const glyph_outline& shape)
{
    std::size_t count = 0;
    for (const auto& contour : shape)
    {
        count += contour.size();
    }
    return count;
}

outline_point indexed_point(const glyph_outline& shape, unsigned index)
{
    for (const auto& contour : shape)
    {
        if (index < contour.size())
        {
            return contour[index];
        }
        index -= static_cast<unsigned>(contour.size());
    }
    throw std::invalid_argument("复合字形对齐点越界");
}

void transform(glyph_outline& shape, double a, double b, double c, double d, double x, double y)
{
    for (auto& contour : shape)
    {
        for (auto& point : contour)
        {
            const double px = point.x * a + point.y * c + x;
            const double py = point.x * b + point.y * d + y;
            if (!std::isfinite(px) || !std::isfinite(py) || std::abs(px) > 1e7 || std::abs(py) > 1e7)
            {
                throw std::invalid_argument("复合字形变换坐标超限");
            }
            point.x = px;
            point.y = py;
        }
    }
}
}

glyph_outline font_face::read_compound(font_reader data, std::vector<std::uint16_t>& chain, std::size_t& budget) const
{
    glyph_outline result;
    std::size_t cursor = 10;
    unsigned flags = 0, components = 0;
    do
    {
        if (++components > 4096)
        {
            throw std::length_error("复合字形组件超过 4096 个");
        }
        flags = data.u16(cursor);
        const auto glyph = data.u16(cursor + 2);
        cursor += 4;
        const bool words = flags & 1, coordinates = flags & 2;
        int first = 0, second = 0;
        if (words)
        {
            first = coordinates ? int(data.i16(cursor)) : int(data.u16(cursor));
            second = coordinates ? int(data.i16(cursor + 2)) : int(data.u16(cursor + 2));
            cursor += 4;
        }
        else
        {
            first = data.u8(cursor++);
            second = data.u8(cursor++);
            if (coordinates)
            {
                first = first >= 128 ? first - 256 : first;
                second = second >= 128 ? second - 256 : second;
            }
        }
        double a = 1, b = 0, c = 0, d = 1;
        const unsigned scale_flags = flags & (8 | 64 | 128);
        if (scale_flags && (scale_flags & (scale_flags - 1)))
        {
            throw std::invalid_argument("复合字形变换标志冲突");
        }
        if (flags & 8)
        {
            a = d = data.i16(cursor) / 16384.0;
            cursor += 2;
        }
        else if (flags & 64)
        {
            a = data.i16(cursor) / 16384.0;
            d = data.i16(cursor + 2) / 16384.0;
            cursor += 4;
        }
        else if (flags & 128)
        {
            a = data.i16(cursor) / 16384.0;
            b = data.i16(cursor + 2) / 16384.0;
            c = data.i16(cursor + 4) / 16384.0;
            d = data.i16(cursor + 6) / 16384.0;
            cursor += 8;
        }
        auto child = read_outline(glyph, chain, budget);
        transform(child, a, b, c, d, 0, 0);
        double x = first, y = second;
        if (!coordinates)
        {
            const auto parent_point = indexed_point(result, first), child_point = indexed_point(child, second);
            x = parent_point.x - child_point.x;
            y = parent_point.y - child_point.y;
        }
        else if (flags & 0x800)
        {
            if (flags & 0x1000)
            {
                throw std::invalid_argument("复合字形偏移缩放标志冲突");
            }
            x = first * a + second * c;
            y = first * b + second * d;
        }
        if (coordinates && (flags & 4))
        {
            x = std::round(x);
            y = std::round(y);
        }
        transform(child, 1, 0, 0, 1, x, y);
        if (point_count(result) + point_count(child) > 262144)
        {
            throw std::length_error("复合字形轮廓点超过 262144 个");
        }
        result.insert(result.end(), std::make_move_iterator(child.begin()), std::make_move_iterator(child.end()));
    }
    while (flags & 32);
    if (flags & 256)
    {
        const auto instructions = data.u16(cursor);
        data.slice(cursor + 2, instructions);
    }
    return result;
}
}
