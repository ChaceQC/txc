#include "stdlib/native_gui/core/unicode.hpp"
#include "stdlib/native_gui/core/unicode_tables.hpp"

namespace tx::ui
{
namespace
{
using property = word_property;

bool ignored(property value)
{
    return value == property::extend || value == property::format || value == property::zwj;
}

bool newline(property value)
{
    return value == property::cr || value == property::lf || value == property::newline;
}

bool letter(property value)
{
    return value == property::aletter || value == property::hebrew_letter;
}

bool middle_letter(property value)
{
    return value == property::midletter || value == property::midnumlet || value == property::single_quote;
}

bool middle_number(property value)
{
    return value == property::midnum || value == property::midnumlet || value == property::single_quote;
}

bool word_part(property value)
{
    return letter(value) || value == property::numeric || value == property::katakana;
}

bool separate(property left, property right, property before, property after, unsigned regional_count)
{
    if (letter(left) && letter(right))
    {
        return false;
    }
    if ((letter(left) && middle_letter(right) && letter(after)) ||
        (letter(before) && middle_letter(left) && letter(right)))
    {
        return false;
    }
    if (left == property::hebrew_letter && right == property::single_quote)
    {
        return false;
    }
    if ((left == property::hebrew_letter && right == property::double_quote && after == property::hebrew_letter) ||
        (before == property::hebrew_letter && left == property::double_quote && right == property::hebrew_letter))
    {
        return false;
    }
    if ((left == property::numeric && right == property::numeric) ||
        (letter(left) && right == property::numeric) || (left == property::numeric && letter(right)))
    {
        return false;
    }
    if ((left == property::numeric && middle_number(right) && after == property::numeric) ||
        (before == property::numeric && middle_number(left) && right == property::numeric))
    {
        return false;
    }
    if (left == property::katakana && right == property::katakana)
    {
        return false;
    }
    if (((word_part(left) || left == property::extendnumlet) && right == property::extendnumlet) ||
        (left == property::extendnumlet && word_part(right)))
    {
        return false;
    }
    return !(left == property::regional_indicator && right == property::regional_indicator && regional_count % 2 == 1);
}
}

std::vector<std::size_t> word_boundaries(std::u32string_view text)
{
    std::vector<property> properties;
    std::vector<std::size_t> previous(text.size()), next(text.size());
    std::size_t last = text.size();
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        const auto value = word_of(text[index]);
        properties.push_back(value);
        previous[index] = last;
        if (!ignored(value))
        {
            last = index;
        }
    }
    last = text.size();
    for (std::size_t index = text.size(); index > 0;)
    {
        --index;
        next[index] = last;
        if (!ignored(properties[index]))
        {
            last = index;
        }
    }
    const auto at = [&](std::size_t index)
    {
        return index == text.size() ? property::other : properties[index];
    };
    std::vector<std::size_t> result{0};
    unsigned regional_count = 0;
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        if (index)
        {
            const auto raw_left = properties[index - 1], right = properties[index];
            bool boundary = true;
            if (raw_left == property::cr && right == property::lf)
            {
                boundary = false;
            }
            else if (newline(raw_left) || newline(right))
            {
                boundary = true;
            }
            else if ((raw_left == property::zwj && pictographic_of(text[index]) == pictographic_property::yes) ||
                (raw_left == property::wsegspace && right == property::wsegspace) || ignored(right))
            {
                boundary = false;
            }
            else
            {
                const auto left = previous[index];
                const auto before = left == text.size() ? text.size() : previous[left];
                boundary = separate(at(left), right, at(before), at(next[index]), regional_count);
            }
            if (boundary)
            {
                result.push_back(index);
            }
        }
        if (!ignored(properties[index]))
        {
            regional_count = properties[index] == property::regional_indicator ? regional_count + 1 : 0;
        }
    }
    if (!text.empty())
    {
        result.push_back(text.size());
    }
    return result;
}
}
