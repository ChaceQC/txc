#pragma once

#include <cstdint>

namespace tx::ui
{
enum class word_property : std::uint8_t
{
    other, lf, newline, cr, wsegspace, double_quote, single_quote, midnum, midnumlet, numeric, midletter, aletter, extendnumlet, format, extend, hebrew_letter, zwj, katakana, regional_indicator
};
word_property word_of(char32_t scalar) noexcept;

enum class grapheme_property : std::uint8_t
{
    other, control, lf, cr, extend, prepend, spacingmark, l, v, t, zwj, lv, lvt, regional_indicator
};
grapheme_property grapheme_of(char32_t scalar) noexcept;

enum class pictographic_property : std::uint8_t
{
    no, yes
};
pictographic_property pictographic_of(char32_t scalar) noexcept;

enum class conjunct_property : std::uint8_t
{
    none, extend, consonant, linker
};
conjunct_property conjunct_of(char32_t scalar) noexcept;

}
