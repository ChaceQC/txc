#include "stdlib/native_gui/core/opentype_internal.hpp"
#include "stdlib/native_gui/core/bidi.hpp"
#include "stdlib/native_gui/core/line_break.hpp"
#include "stdlib/native_gui/core/unicode_tables.hpp"
#include "stdlib/native_gui/core/unicode.hpp"
#include <algorithm>

namespace tx::ui
{
namespace
{
bool invisible(char32_t scalar)
{
    return bidi_invisible(scalar) || hard_line_break(scalar) || scalar == U'\t' ||
        scalar == 0x200d || scalar == 0x200b || scalar == 0x2060 ||
        scalar == 0xfeff || (scalar >= 0xfe00 && scalar <= 0xfe0f) ||
        (scalar >= 0xe0100 && scalar <= 0xe01ef);
}
}

std::vector<shaped_glyph> shape_text(const font_face& font, std::u32string_view text,
    double size, bool right_to_left)
{
    const auto scale = font.em_scale(size);
    if (text.size() > 1000000)
    {
        throw std::length_error("单次整形文本超过一百万标量");
    }
    shaping_options options;
    options.right_to_left = right_to_left;
    for (auto scalar : text)
    {
        if (const auto script = shaping_script(scalar))
        {
            options.script = script;
            break;
        }
    }
    const auto forms = options.script == ot_tag("arab") ? arabic_forms(text) : std::vector<std::uint32_t>(text.size());
    const auto boundaries = grapheme_boundaries(text);
    std::vector<shaped_glyph> result;
    const auto gdef = font.layout_table("GDEF");
    const auto class_table = gdef.size() && gdef.u16(4) ? ot::subtable(gdef, gdef.u16(4)) : font_reader({});
    for (std::size_t cluster = 0; cluster + 1 < boundaries.size(); ++cluster)
    {
        for (auto i = boundaries[cluster]; i < boundaries[cluster + 1]; ++i)
        {
            if (text[i] == 0x200c)
            {
                // ZWNJ 必须阻断连字匹配，但不绘制 .notdef 或贡献宽度。
                result.push_back({0, boundaries[cluster], boundaries[cluster + 1], 0, 0, 0, 1, 0, {}, true});
                continue;
            }
            if (invisible(text[i]))
            {
                continue;
            }
            const auto glyph = font.glyph(right_to_left ? bidi_mirror(text[i]) : text[i]);
            const auto defined_class = ot::class_of(class_table, glyph);
            const bool mark = defined_class == 3 || grapheme_of(text[i]) == grapheme_property::extend;
            result.push_back({glyph, boundaries[cluster], boundaries[cluster + 1], 0, 0, 0,
                defined_class ? defined_class : mark ? 3u : 1u, forms[i], {}});
        }
    }
    substitute_glyphs(font.layout_table("GSUB"), gdef, font.glyph_count(), result, options);
    for (auto& glyph : result)
    {
        const auto kind = ot::class_of(class_table, glyph.glyph);
        if (kind)
        {
            glyph.glyph_class = kind;
        }
        glyph.advance = glyph.glyph_class == 3 || glyph.invisible ? 0 : font.advance(glyph.glyph, size);
    }
    // 无 mark 查找时仍保留此前的基字形起点降级定位；GPOS 成功附着会覆盖它。
    double origin = 0, base_origin = 0;
    for (auto& glyph : result)
    {
        if (glyph.glyph_class == 3)
        {
            glyph.x = right_to_left ? 0 : base_origin - origin;
        }
        else
        {
            base_origin = origin;
        }
        origin += glyph.advance;
    }
    position_glyphs(font.layout_table("GPOS"), gdef, font.glyph_count(), result, options, scale, size);
    for (const auto& glyph : result)
    {
        if (!std::isfinite(glyph.advance) || !std::isfinite(glyph.x) || !std::isfinite(glyph.y))
        {
            throw std::invalid_argument("整形产生非有限位置");
        }
    }
    return result;
}
}
