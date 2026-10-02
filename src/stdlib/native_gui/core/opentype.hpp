#pragma once

#include "stdlib/native_gui/core/font.hpp"

namespace tx::ui
{
// 全部位置均为逻辑 Unicode 标量区间；多个输出字形可共享同一区间。
struct shaped_glyph
{
    std::uint16_t glyph = 0;
    std::size_t begin = 0, end = 0;
    double advance = 0, x = 0, y = 0;
    unsigned glyph_class = 1;
    std::uint32_t form = 0;
    std::vector<std::size_t> components;
    bool invisible = false;
};

constexpr std::uint32_t ot_tag(std::string_view name)
{
    return (std::uint32_t(name[0]) << 24) | (std::uint32_t(name[1]) << 16) |
        (std::uint32_t(name[2]) << 8) | std::uint32_t(name[3]);
}

struct shaping_options
{
    std::uint32_t script = ot_tag("DFLT"), language = 0;
    bool right_to_left = false;
};

// 独立表入口也用于不依赖系统字体版本的二进制格式检查。
void substitute_glyphs(font_reader gsub, font_reader gdef, unsigned glyph_count,
    std::vector<shaped_glyph>& glyphs, shaping_options options);
void position_glyphs(font_reader gpos, font_reader gdef, unsigned glyph_count,
    std::vector<shaped_glyph>& glyphs, shaping_options options, double scale, double size);
std::vector<shaped_glyph> shape_text(const font_face& font, std::u32string_view text,
    double size, bool right_to_left);
std::uint32_t shaping_script(char32_t scalar);
std::vector<std::uint32_t> arabic_forms(std::u32string_view text);
std::vector<double> ligature_carets(font_reader gdef, std::uint16_t glyph, double scale);
}
