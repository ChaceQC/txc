#pragma once

#include "stdlib/native_gui/core/text_layout.hpp"

namespace tx::ui
{
struct layout_cluster
{
    std::size_t begin, end;
    double advance = 0;
    bool newline = false, space = false;
    const font_face* face = nullptr;
    std::uint32_t script = 0;
    std::vector<positioned_glyph> glyphs;
};
std::vector<layout_cluster> text_clusters(const font_family& fonts, std::u32string_view text,
    const std::vector<unsigned>& levels);
void shape_clusters(std::vector<layout_cluster>& clusters, std::u32string_view text,
    std::size_t start, std::size_t end, const std::vector<unsigned>& levels, std::size_t offset, double size);
}
