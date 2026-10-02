#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace tx::ui
{
enum class bidi_type
{
    l, r, al, en, es, et, an, cs, nsm, bn, b, s, ws, on,
    lre, lro, rle, rlo, pdf, lri, rli, fsi, pdi
};

bidi_type bidi_property(char32_t scalar);
char32_t bidi_mirror(char32_t scalar);
bool bidi_removed(bidi_type type);
bool bidi_invisible(char32_t scalar);

struct bidi_paragraph
{
    unsigned base = 0;
    std::vector<bidi_type> original;
    std::vector<unsigned> levels;
    // L1 按实际软换行边界执行；X9 控制符不进入视觉序列。
    std::vector<unsigned> line_levels(std::size_t begin, std::size_t end) const;
    std::vector<std::size_t> visual_order(std::size_t begin, std::size_t end) const;
};

// direction: -1 自动，0 左至右，1 右至左。输入为单个段落。
bidi_paragraph resolve_bidi(std::u32string_view text, int direction = -1);
bidi_paragraph resolve_bidi_types(std::span<const bidi_type> types, int direction = -1,
    std::u32string_view text = {});
}
