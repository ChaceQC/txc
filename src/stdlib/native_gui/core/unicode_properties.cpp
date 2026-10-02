#include "stdlib/native_gui/core/unicode_tables.hpp"

#include <algorithm>
#include <iterator>

namespace tx::ui
{
namespace
{
template<class property>
struct property_range
{
    char32_t first, last;
    property value;
};

#include "stdlib/native_gui/core/unicode_tables.inc"
#include "stdlib/native_gui/core/word_tables.inc"

template<class property, std::size_t count>
property lookup(const property_range<property> (&ranges)[count], char32_t value, property fallback) noexcept
{
    const auto found = std::lower_bound(std::begin(ranges), std::end(ranges), value,
        [](const auto& range, char32_t scalar)
        {
            return range.last < scalar;
        });
    return found != std::end(ranges) && found->first <= value ? found->value : fallback;
}
}

grapheme_property grapheme_of(char32_t scalar) noexcept
{
    return lookup(grapheme_ranges, scalar, grapheme_property::other);
}

word_property word_of(char32_t scalar) noexcept
{
    return lookup(word_ranges, scalar, word_property::other);
}

pictographic_property pictographic_of(char32_t scalar) noexcept
{
    return lookup(pictographic_ranges, scalar, pictographic_property::no);
}

conjunct_property conjunct_of(char32_t scalar) noexcept
{
    return lookup(conjunct_ranges, scalar, conjunct_property::none);
}
}
