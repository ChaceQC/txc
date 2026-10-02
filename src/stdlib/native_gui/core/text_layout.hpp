#pragma once

#include "stdlib/native_gui/core/font.hpp"
#include "stdlib/native_gui/core/raster.hpp"
#include "stdlib/native_gui/core/unicode.hpp"

namespace tx::ui
{
struct positioned_glyph
{
    std::uint16_t glyph = 0;
    point baseline;
};

struct positioned_cluster
{
    std::size_t begin = 0, end = 0;
    rect bounds;
};

struct text_line
{
    std::size_t begin = 0, end = 0;
    double width = 0, y = 0;
};

class text_layout
{
public:
    text_layout(const font_face& font, std::u32string_view text, double size, double width, bool wrap);
    double width() const noexcept;
    double height() const noexcept;
    std::size_t hit(point position) const;
    rect caret(std::size_t index) const;
    std::vector<rect> selection(std::size_t begin, std::size_t end) const;
    void draw(rasterizer& painter, point origin, color color) const;
    const std::vector<text_line>& lines() const noexcept;
private:
    const font_face& font_;
    double size_ = 0, width_ = 0, height_ = 0, line_height_ = 0;
    std::size_t length_ = 0;
    std::vector<positioned_glyph> glyphs_;
    std::vector<positioned_cluster> clusters_;
    std::vector<text_line> lines_;
    void append(std::u32string_view text, std::size_t begin, std::size_t end, double x, double y, double width);
};
}
