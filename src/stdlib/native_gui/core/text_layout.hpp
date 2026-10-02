#pragma once

#include "stdlib/native_gui/core/font.hpp"
#include "stdlib/native_gui/core/font_family.hpp"
#include "stdlib/native_gui/core/raster.hpp"
#include "stdlib/native_gui/core/unicode.hpp"

namespace tx::ui
{
enum class caret_affinity
{
    upstream, downstream
};

struct text_position
{
    std::size_t index = 0;
    caret_affinity affinity = caret_affinity::downstream;
    bool operator==(const text_position&) const = default;
};

struct positioned_glyph
{
    std::uint16_t glyph = 0;
    point baseline;
    const font_face* face = nullptr;
};

struct positioned_cluster
{
    std::size_t begin = 0, end = 0;
    rect bounds;
    bool right_to_left = false;
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
    text_layout(const font_family& fonts, std::u32string_view text, double size, double width, bool wrap);
    double width() const noexcept;
    double height() const noexcept;
    std::size_t hit(point position) const;
    rect caret(std::size_t index) const;
    text_position hit_position(point position) const;
    rect caret(text_position position) const;
    std::optional<rect> alternate_caret(text_position position) const;
    text_position move_visual(text_position position, int direction) const;
    text_position line_edge(text_position position, int direction) const;
    text_position selection_edge(std::size_t begin, std::size_t end, int direction) const;
    std::vector<rect> selection(std::size_t begin, std::size_t end) const;
    void draw(rasterizer& painter, point origin, color color) const;
    const std::vector<text_line>& lines() const noexcept;
private:
    font_family fonts_;
    double size_ = 0, width_ = 0, height_ = 0, line_height_ = 0;
    std::size_t length_ = 0;
    std::vector<positioned_glyph> glyphs_;
    std::vector<positioned_cluster> clusters_;
    std::vector<text_line> lines_;
    struct caret_stop
    {
        text_position position;
        rect bounds;
    };
    std::vector<caret_stop> stops_;
    void build_carets();
};
}
