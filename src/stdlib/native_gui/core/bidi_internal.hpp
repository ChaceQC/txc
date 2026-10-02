#pragma once
#include "stdlib/native_gui/core/bidi.hpp"

namespace tx::ui
{
struct bracket_entry
{
    char32_t scalar, paired;
    bool opening;
};
const bracket_entry* bidi_bracket(char32_t scalar);
bool bidi_isolate(bidi_type type);
bidi_type bidi_direction(unsigned level);
void resolve_bidi_sequence(std::vector<bidi_type>& types, std::vector<unsigned>& levels,
    const std::vector<std::size_t>& sequence, bidi_type sos, bidi_type eos, std::u32string_view text);
}
