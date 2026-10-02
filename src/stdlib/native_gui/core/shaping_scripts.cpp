#include "stdlib/native_gui/core/opentype.hpp"
#include <algorithm>

namespace tx::ui
{
namespace
{
struct joining_range
{
    char32_t first, last;
    char value;
};
struct script_range
{
    char32_t first, last;
    std::uint32_t value;
};
#include "stdlib/native_gui/core/shaping_tables.inc"

char joining_type(char32_t scalar)
{
    const auto found = std::lower_bound(std::begin(joining_ranges), std::end(joining_ranges), scalar,
        [](const auto& range, char32_t value)
        {
            return range.last < value;
        });
    return found != std::end(joining_ranges) && found->first <= scalar ? found->value : 'u';
}
}

std::uint32_t shaping_script(char32_t scalar)
{
    const auto found = std::lower_bound(std::begin(script_ranges), std::end(script_ranges), scalar,
        [](const auto& range, char32_t value)
        {
            return range.last < value;
        });
    return found != std::end(script_ranges) && found->first <= scalar ? found->value : 0;
}

std::vector<std::uint32_t> arabic_forms(std::u32string_view text)
{
    std::vector<std::uint32_t> result(text.size());
    std::vector<std::size_t> significant;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        if (joining_type(text[i]) != 't')
        {
            significant.push_back(i);
        }
    }
    for (std::size_t i = 0; i < significant.size(); ++i)
    {
        const auto at = significant[i];
        const auto current = joining_type(text[at]);
        const auto previous = i ? joining_type(text[significant[i - 1]]) : 'u';
        const auto next = i + 1 < significant.size() ? joining_type(text[significant[i + 1]]) : 'u';
        const bool before = (current == 'r' || current == 'd' || current == 'c') &&
            (previous == 'l' || previous == 'd' || previous == 'c');
        const bool after = (current == 'l' || current == 'd' || current == 'c') &&
            (next == 'r' || next == 'd' || next == 'c');
        if (current != 'u' && current != 'c')
        {
            result[at] = before ? (after ? ot_tag("medi") : ot_tag("fina")) :
                (after ? ot_tag("init") : ot_tag("isol"));
        }
    }
    return result;
}
}
