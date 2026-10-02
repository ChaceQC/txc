#include "stdlib/native_gui/core/opentype_internal.hpp"
#include <algorithm>

namespace tx::ui::ot
{
namespace
{
application cursive(processor& engine, const lookup& entry, font_reader data, std::size_t position)
{
    const auto next = engine.next(entry, position);
    if (!next)
    {
        return {};
    }
    const auto cover = subtable(data, data.u16(2));
    const auto current_index = coverage(cover, engine.glyphs[position].glyph);
    const auto next_index = coverage(cover, engine.glyphs[*next].glyph);
    if (next_index < 0)
    {
        return {};
    }
    if (current_index >= data.u16(4) || next_index >= data.u16(4))
    {
        throw std::invalid_argument("GPOS Cursive 覆盖索引越界");
    }
    const auto exit = engine.anchor(data, data.u16(8 + current_index * 4));
    const auto enter = engine.anchor(data, data.u16(6 + next_index * 4));
    if (!exit || !enter)
    {
        return {};
    }
    auto& first = engine.glyphs[position];
    auto& second = engine.glyphs[*next];
    const auto dx = first.x + exit->x - second.x - enter->x - engine.origin(position, *next);
    if (engine.options.right_to_left)
    {
        second.advance -= dx;
    }
    else
    {
        first.advance += dx;
    }
    if (entry.flags & 1)
    {
        engine.cursive_link(position, *next, enter->y - exit->y);
    }
    else
    {
        engine.cursive_link(*next, position, exit->y - enter->y);
    }
    return {true, *next};
}
}

void processor::cursive_link(std::size_t child, std::size_t parent, double delta)
{
    cursive_parents_[child] = parent;
    cursive_deltas_[child] = delta;
}

void processor::finish_cursive()
{
    // 每次查找结束后线性求解附着链，避免逐个回移已连接前缀产生平方复杂度。
    std::vector<unsigned char> states(cursive_parents_.size());
    for (std::size_t start = 0; start < states.size(); ++start)
    {
        if (states[start])
        {
            continue;
        }
        std::vector<std::size_t> path;
        auto at = start;
        while (at < states.size() && states[at] == 0)
        {
            spend();
            states[at] = 1;
            path.push_back(at);
            at = cursive_parents_[at];
        }
        if (at < states.size() && states[at] == 1)
        {
            throw std::invalid_argument("GPOS Cursive 附着链形成循环");
        }
        for (auto i = path.size(); i > 0; --i)
        {
            const auto child = path[i - 1], parent = cursive_parents_[child];
            if (parent < states.size())
            {
                glyphs[child].y = glyphs[parent].y + cursive_deltas_[child];
            }
            states[child] = 2;
        }
    }
    std::fill(cursive_parents_.begin(), cursive_parents_.end(), glyphs.size());
}

application processor::attach(const lookup& entry, font_reader data, unsigned type, std::size_t position)
{
    if (data.u16(0) != 1)
    {
        throw std::invalid_argument("GPOS Attachment 格式非法");
    }
    if (type == 3)
    {
        return cursive(*this, entry, data, position);
    }
    auto previous = next(entry, position, -1);
    while (previous && type != 6 && glyph_class(glyphs[*previous]) == 3)
    {
        previous = next(entry, *previous, -1);
    }
    if (!previous)
    {
        return {};
    }
    const auto covered = coverage(subtable(data, data.u16(4)), glyphs[*previous].glyph);
    if (covered < 0)
    {
        return {};
    }
    const auto index = coverage(subtable(data, data.u16(2)), glyphs[position].glyph);
    const auto marks = subtable(data, data.u16(8)), bases = subtable(data, data.u16(10));
    const auto classes = data.u16(6);
    if (index >= marks.u16(0) || covered >= bases.u16(0))
    {
        throw std::invalid_argument("GPOS 标记或基字形覆盖索引越界");
    }
    const auto mark_class = marks.u16(2 + index * 4);
    if (mark_class >= classes)
    {
        throw std::invalid_argument("GPOS 标记类别越界");
    }
    const auto mark = anchor(marks, marks.u16(4 + index * 4));
    std::optional<point> base;
    if (type == 5)
    {
        const auto ligature = subtable(bases, bases.u16(2 + covered * 2));
        const auto count = ligature.u16(0);
        if (!count)
        {
            throw std::invalid_argument("GPOS 连字组件为空");
        }
        unsigned component = count - 1;
        const auto& starts = glyphs[*previous].components;
        if (!starts.empty())
        {
            const auto found = std::upper_bound(starts.begin(), starts.end(), glyphs[position].begin);
            component = std::min<unsigned>(count - 1, found == starts.begin() ? 0 : unsigned(found - starts.begin() - 1));
        }
        base = anchor(ligature, ligature.u16(2 + (std::size_t(component) * classes + mark_class) * 2));
    }
    else
    {
        base = anchor(bases, bases.u16(2 + (std::size_t(covered) * classes + mark_class) * 2));
    }
    if (!mark || !base)
    {
        return {};
    }
    glyphs[position].x = glyphs[*previous].x + base->x - mark->x - origin(*previous, position);
    glyphs[position].y = glyphs[*previous].y + base->y - mark->y;
    return {true, position + 1};
}
}
