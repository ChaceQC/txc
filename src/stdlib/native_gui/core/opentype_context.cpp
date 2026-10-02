#include "stdlib/native_gui/core/opentype_internal.hpp"

namespace tx::ui::ot
{
namespace
{
struct pattern
{
    font_reader parent, data, classes;
    unsigned format;
    std::size_t offset;
    unsigned count;
    bool matches(unsigned index, std::uint16_t glyph) const
    {
        const auto expected = data.u16(offset + index * 2);
        if (format == 3)
        {
            return coverage(subtable(parent, expected), glyph) >= 0;
        }
        return (format == 2 ? class_of(classes, glyph) : glyph) == expected;
    }
};

bool match(processor& engine, const lookup& entry, const pattern& sequence, std::size_t& at,
    int direction, bool include_first)
{
    for (unsigned i = 0; i < sequence.count; ++i)
    {
        engine.spend();
        if (i || !include_first)
        {
            const auto next = engine.next(entry, at, direction);
            if (!next)
            {
                return false;
            }
            at = *next;
        }
        if (!sequence.matches(i, engine.glyphs[at].glyph))
        {
            return false;
        }
    }
    return true;
}

application actions(processor& engine, const lookup& entry, font_reader data, std::size_t offset,
    unsigned count, std::size_t position, std::size_t end, unsigned depth)
{
    data.slice(offset, std::size_t(count) * 4);
    for (unsigned i = 0; i < count; ++i)
    {
        engine.spend();
        auto at = position;
        const auto index = data.u16(offset + i * 4);
        // GSUB 规范：后续 sequenceIndex 相对于前面替换完成后的序列。
        for (unsigned j = 0; j < index && at < end; ++j)
        {
            const auto next = engine.next(entry, at);
            at = next.value_or(end);
        }
        if (at >= end || at >= engine.glyphs.size())
        {
            throw std::invalid_argument("OpenType 上下文动作的序列索引越界");
        }
        const auto previous = engine.glyphs.size();
        engine.apply(data.u16(offset + i * 4 + 2), at, depth + 1);
        end = end + engine.glyphs.size() - previous;
    }
    return {true, end};
}

application rule(processor& engine, const lookup& entry, font_reader parent, font_reader data,
    unsigned format, bool chained, std::size_t position, unsigned depth,
    font_reader before_class, font_reader input_class, font_reader after_class)
{
    std::size_t offset = format == 3 ? 2 : 0;
    auto before = position;
    if (chained)
    {
        const auto count = data.u16(offset);
        offset += 2;
        data.slice(offset, std::size_t(count) * 2);
        if (!match(engine, entry, {parent, data, before_class, format, offset, count}, before, -1, false))
        {
            return {};
        }
        offset += count * 2;
    }
    const auto count = data.u16(offset);
    if (!count)
    {
        throw std::invalid_argument("OpenType 上下文输入数量为零");
    }
    offset += 2;
    unsigned action_count = 0;
    if (!chained)
    {
        action_count = data.u16(offset);
        offset += 2;
    }
    const unsigned stored = format == 3 ? count : count - 1;
    data.slice(offset, std::size_t(stored) * 2);
    auto last = position;
    if (!match(engine, entry, {parent, data, input_class, format, offset, stored}, last, 1, format == 3))
    {
        return {};
    }
    offset += stored * 2;
    if (chained)
    {
        const auto after_count = data.u16(offset);
        offset += 2;
        data.slice(offset, std::size_t(after_count) * 2);
        auto after = last;
        if (!match(engine, entry, {parent, data, after_class, format, offset, after_count}, after, 1, false))
        {
            return {};
        }
        offset += after_count * 2;
        action_count = data.u16(offset);
        offset += 2;
    }
    return actions(engine, entry, data, offset, action_count, position, last + 1, depth);
}
}

application processor::context(const lookup& entry, font_reader data, bool chained,
    std::size_t position, unsigned depth)
{
    const auto format = data.u16(0);
    const font_reader empty({});
    if (format == 3)
    {
        return rule(*this, entry, data, data, format, chained, position, depth, empty, empty, empty);
    }
    if (format != 1 && format != 2)
    {
        throw std::invalid_argument("OpenType Context 格式非法");
    }
    const auto covered = coverage(subtable(data, data.u16(2)), glyphs[position].glyph);
    if (covered < 0)
    {
        return {};
    }
    auto before_class = empty, input_class = empty, after_class = empty;
    unsigned count_offset = 4, index = covered;
    if (format == 2)
    {
        before_class = chained && data.u16(4) ? subtable(data, data.u16(4)) : empty;
        input_class = subtable(data, data.u16(chained ? 6 : 4));
        after_class = chained && data.u16(8) ? subtable(data, data.u16(8)) : empty;
        count_offset = chained ? 10 : 6;
        index = class_of(input_class, glyphs[position].glyph);
    }
    if (index >= data.u16(count_offset))
    {
        return {};
    }
    const auto offset = data.u16(count_offset + 2 + index * 2);
    if (!offset)
    {
        return {};
    }
    const auto set = subtable(data, offset);
    set.slice(2, std::size_t(set.u16(0)) * 2);
    for (unsigned i = 0; i < set.u16(0); ++i)
    {
        spend();
        const auto result = rule(*this, entry, data, subtable(set, set.u16(2 + i * 2)),
            format, chained, position, depth, before_class, input_class, after_class);
        if (result.matched)
        {
            return result;
        }
    }
    return {};
}
}
