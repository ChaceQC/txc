#include "stdlib/native_gui/core/bidi_internal.hpp"

#include <algorithm>
#include <iterator>

namespace tx::ui
{
namespace
{
struct bidi_range
{
    char32_t first, last;
    bidi_type type;
};
struct mirror_entry
{
    char32_t scalar, mirror;
};
#include "stdlib/native_gui/core/bidi_tables.inc"
}

bidi_type bidi_property(char32_t scalar)
{
    const auto found = std::lower_bound(std::begin(bidi_ranges), std::end(bidi_ranges), scalar,
        [](const auto& range, char32_t value)
        {
            return range.last < value;
        });
    return found == std::end(bidi_ranges) ? bidi_type::l : found->type;
}

const bracket_entry* bidi_bracket(char32_t scalar)
{
    const auto found = std::lower_bound(std::begin(bidi_brackets), std::end(bidi_brackets), scalar,
        [](const auto& entry, char32_t value)
        {
            return entry.scalar < value;
        });
    return found != std::end(bidi_brackets) && found->scalar == scalar ? found : nullptr;
}

char32_t bidi_mirror(char32_t scalar)
{
    const auto found = std::lower_bound(std::begin(bidi_mirrors), std::end(bidi_mirrors), scalar,
        [](const auto& entry, char32_t value)
        {
            return entry.scalar < value;
        });
    return found != std::end(bidi_mirrors) && found->scalar == scalar ? found->mirror : scalar;
}

bool bidi_isolate(bidi_type type)
{
    return type == bidi_type::lri || type == bidi_type::rli || type == bidi_type::fsi;
}

bool bidi_removed(bidi_type type)
{
    return type == bidi_type::lre || type == bidi_type::rle || type == bidi_type::lro ||
        type == bidi_type::rlo || type == bidi_type::pdf || type == bidi_type::bn;
}

bool bidi_invisible(char32_t scalar)
{
    const auto type = bidi_property(scalar);
    return bidi_removed(type) || bidi_isolate(type) || type == bidi_type::pdi ||
        scalar == 0x200e || scalar == 0x200f || scalar == 0x061c;
}

bidi_type bidi_direction(unsigned level)
{
    return level % 2 ? bidi_type::r : bidi_type::l;
}
}
