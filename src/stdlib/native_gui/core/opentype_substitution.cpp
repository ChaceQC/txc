#include "stdlib/native_gui/core/opentype_internal.hpp"
#include <algorithm>

namespace tx::ui::ot
{
namespace
{
application single(processor& engine, font_reader data, std::size_t position, int covered)
{
    auto& glyph = engine.glyphs[position];
    const auto format = data.u16(0);
    unsigned replacement = 0;
    if (format == 1)
    {
        replacement = (glyph.glyph + data.i16(4)) & 0xffff;
    }
    else if (format == 2 && covered < data.u16(4))
    {
        replacement = data.u16(6 + covered * 2);
    }
    else
    {
        throw std::invalid_argument("GSUB Single 格式或覆盖索引非法");
    }
    engine.check_glyph(replacement);
    glyph.glyph = static_cast<std::uint16_t>(replacement);
    return {true, position + 1};
}

application multiple(processor& engine, font_reader data, unsigned type, std::size_t position, int covered)
{
    if (data.u16(0) != 1 || covered >= data.u16(4))
    {
        throw std::invalid_argument("GSUB Sequence/Alternate 格式或覆盖索引非法");
    }
    const auto sequence = subtable(data, data.u16(6 + covered * 2));
    const auto count = sequence.u16(0);
    sequence.slice(2, std::size_t(count) * 2);
    if (type == 3)
    {
        if (!count)
        {
            throw std::invalid_argument("GSUB Alternate 集合为空");
        }
        engine.check_glyph(sequence.u16(2));
        engine.glyphs[position].glyph = sequence.u16(2);
        return {true, position + 1};
    }
    if (engine.glyphs.size() + count > 1000001)
    {
        throw std::length_error("GSUB 字形展开超过限制");
    }
    std::vector<shaped_glyph> replacement(count, engine.glyphs[position]);
    for (unsigned i = 0; i < count; ++i)
    {
        engine.spend();
        replacement[i].glyph = sequence.u16(2 + i * 2);
        engine.check_glyph(replacement[i].glyph);
        replacement[i].components.clear();
        replacement[i].glyph_class = 1;
    }
    auto at = engine.glyphs.erase(engine.glyphs.begin() + position);
    engine.glyphs.insert(at, replacement.begin(), replacement.end());
    return {true, position + count};
}

application ligature(processor& engine, const lookup& entry, font_reader data, std::size_t position, int covered)
{
    if (data.u16(0) != 1 || covered >= data.u16(4))
    {
        throw std::invalid_argument("GSUB Ligature 格式或覆盖索引非法");
    }
    const auto set = subtable(data, data.u16(6 + covered * 2));
    set.slice(2, std::size_t(set.u16(0)) * 2);
    for (unsigned i = 0; i < set.u16(0); ++i)
    {
        engine.spend();
        const auto lig = subtable(set, set.u16(2 + i * 2));
        const auto count = lig.u16(2);
        if (!count)
        {
            throw std::invalid_argument("GSUB 连字组件数量为零");
        }
        lig.slice(4, std::size_t(count - 1) * 2);
        std::vector<std::size_t> matched{position};
        for (unsigned j = 1; j < count; ++j)
        {
            const auto at = engine.next(entry, matched.back());
            if (!at || engine.glyphs[*at].glyph != lig.u16(4 + (j - 1) * 2))
            {
                break;
            }
            matched.push_back(*at);
        }
        if (matched.size() != count)
        {
            continue;
        }
        engine.check_glyph(lig.u16(0));
        auto replacement = engine.glyphs[position];
        replacement.glyph = lig.u16(0);
        replacement.glyph_class = 2;
        replacement.components.clear();
        for (auto at : matched)
        {
            const auto& component = engine.glyphs[at];
            replacement.begin = std::min(replacement.begin, component.begin);
            replacement.end = std::max(replacement.end, component.end);
            if (component.components.empty())
            {
                replacement.components.push_back(component.begin);
            }
            else
            {
                replacement.components.insert(replacement.components.end(), component.components.begin(), component.components.end());
            }
        }
        engine.glyphs[position] = std::move(replacement);
        for (auto j = matched.size(); j > 1; --j)
        {
            engine.glyphs.erase(engine.glyphs.begin() + matched[j - 1]);
        }
        return {true, position + 1};
    }
    return {};
}

application reverse(processor& engine, const lookup& entry, font_reader data, std::size_t position, int covered)
{
    if (data.u16(0) != 1)
    {
        throw std::invalid_argument("GSUB ReverseChain 格式非法");
    }
    std::size_t offset = 4;
    for (int direction : {-1, 1})
    {
        const auto count = data.u16(offset);
        offset += 2;
        data.slice(offset, std::size_t(count) * 2);
        auto at = position;
        for (unsigned i = 0; i < count; ++i)
        {
            const auto next = engine.next(entry, at, direction);
            if (!next || coverage(subtable(data, data.u16(offset + i * 2)), engine.glyphs[*next].glyph) < 0)
            {
                return {};
            }
            at = *next;
        }
        offset += count * 2;
    }
    if (covered >= data.u16(offset))
    {
        throw std::invalid_argument("GSUB ReverseChain 覆盖索引越界");
    }
    const auto replacement = data.u16(offset + 2 + covered * 2);
    engine.check_glyph(replacement);
    engine.glyphs[position].glyph = replacement;
    return {true, position + 1};
}
}

application processor::substitution(const lookup& entry, font_reader data, unsigned type,
    std::size_t position, unsigned depth)
{
    if (type == 5 || type == 6)
    {
        return context(entry, data, type == 6, position, depth);
    }
    const auto covered = coverage(subtable(data, data.u16(2)), glyphs[position].glyph);
    if (covered < 0)
    {
        return {};
    }
    switch (type)
    {
    case 1:
        return single(*this, data, position, covered);
    case 2:
    case 3:
        return multiple(*this, data, type, position, covered);
    case 4:
        return ligature(*this, entry, data, position, covered);
    case 8:
        return reverse(*this, entry, data, position, covered);
    default:
        throw std::invalid_argument("GSUB Lookup 类型非法");
    }
}
}
