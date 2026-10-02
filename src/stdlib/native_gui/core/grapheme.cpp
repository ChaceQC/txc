#include "stdlib/native_gui/core/unicode.hpp"
#include "stdlib/native_gui/core/unicode_tables.hpp"

namespace tx::ui
{
namespace
{
using property = grapheme_property;

bool control(property value)
{
    return value == property::control || value == property::cr || value == property::lf;
}

bool conjunct(std::u32string_view text, std::size_t index)
{
    if (conjunct_of(text[index]) != conjunct_property::consonant)
    {
        return false;
    }
    bool linker = false;
    while (index > 0)
    {
        const auto value = conjunct_of(text[--index]);
        if (value == conjunct_property::consonant)
        {
            return linker;
        }
        if (value != conjunct_property::extend && value != conjunct_property::linker)
        {
            return false;
        }
        linker = linker || value == conjunct_property::linker;
    }
    return false;
}

bool emoji_sequence(std::u32string_view text, std::size_t index)
{
    if (pictographic_of(text[index]) != pictographic_property::yes ||
        grapheme_of(text[index - 1]) != property::zwj)
    {
        return false;
    }
    --index;
    while (index > 0 && grapheme_of(text[index - 1]) == property::extend)
    {
        --index;
    }
    return index > 0 && pictographic_of(text[index - 1]) == pictographic_property::yes;
}

bool boundary(std::u32string_view text, std::size_t index, unsigned regional_count)
{
    const auto previous = grapheme_of(text[index - 1]), current = grapheme_of(text[index]);
    // UAX #29 GB3–GB9b 的顺序有意义：Control 必须先于 Extend/Prepend 处理。
    if (previous == property::cr && current == property::lf)
    {
        return false;
    }
    if (control(previous) || control(current))
    {
        return true;
    }
    if (previous == property::l && (current == property::l || current == property::v ||
        current == property::lv || current == property::lvt))
    {
        return false;
    }
    if ((previous == property::lv || previous == property::v) &&
        (current == property::v || current == property::t))
    {
        return false;
    }
    if ((previous == property::lvt || previous == property::t) && current == property::t)
    {
        return false;
    }
    if (current == property::extend || current == property::zwj || current == property::spacingmark ||
        previous == property::prepend || conjunct(text, index) || emoji_sequence(text, index))
    {
        return false;
    }
    return !(previous == property::regional_indicator && current == property::regional_indicator &&
        regional_count % 2 == 1);
}
}

std::vector<std::size_t> grapheme_boundaries(std::u32string_view text)
{
    std::vector<std::size_t> result{0};
    unsigned regional_count = 0;
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        if (index && boundary(text, index, regional_count))
        {
            result.push_back(index);
        }
        regional_count = grapheme_of(text[index]) == property::regional_indicator ? regional_count + 1 : 0;
    }
    if (!text.empty())
    {
        result.push_back(text.size());
    }
    return result;
}
}
