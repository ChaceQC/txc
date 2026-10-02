#pragma once

#include "stdlib/native_gui/core/geometry.hpp"

#include <filesystem>
#include <deque>
#include <map>
#include <span>
#include <string_view>

namespace tx::ui
{
class font_reader
{
public:
    explicit font_reader(std::span<const std::uint8_t> data);
    std::uint8_t u8(std::size_t offset) const;
    std::uint16_t u16(std::size_t offset) const;
    std::int16_t i16(std::size_t offset) const;
    std::uint32_t u32(std::size_t offset) const;
    std::span<const std::uint8_t> slice(std::size_t offset, std::size_t length) const;
    std::size_t size() const noexcept;
private:
    std::span<const std::uint8_t> data_;
};

struct outline_point
{
    double x, y;
    bool on_curve;
};

using glyph_contour = std::vector<outline_point>;
using glyph_outline = std::vector<glyph_contour>;

class font_face
{
public:
    explicit font_face(std::vector<std::uint8_t> data, unsigned face_index = 0);
    static font_face load(const std::filesystem::path& file, unsigned face_index = 0);
    std::uint16_t glyph(char32_t scalar) const;
    double advance(std::uint16_t glyph, double size) const;
    double ascender(double size) const;
    double line_height(double size) const;
    path outline(std::uint16_t glyph, double size, point baseline) const;
private:
    struct table_entry
    {
        std::uint32_t offset, length;
    };
    std::vector<std::uint8_t> data_;
    std::map<std::uint32_t, table_entry> tables_;
    unsigned units_ = 0, glyph_count_ = 0, metrics_count_ = 0;
    std::int16_t ascender_ = 0, descender_ = 0, line_gap_ = 0;
    bool long_locations_ = false;
    std::size_t cmap_offset_ = 0;
    unsigned cmap_format_ = 0;
    // 字体实例属于 UI 线程，缓存同时限制字形数量和轮廓点总量。
    mutable std::map<std::uint16_t, glyph_outline> outline_cache_;
    mutable std::deque<std::uint16_t> cache_order_;
    mutable std::size_t cached_points_ = 0;
    font_reader table(std::string_view name) const;
    void read_directory(unsigned face_index);
    void read_metrics();
    void choose_cmap();
    const glyph_outline& cached_outline(std::uint16_t glyph) const;
    glyph_outline read_outline(std::uint16_t glyph, std::vector<std::uint16_t>& chain, std::size_t& budget) const;
    glyph_outline read_simple(font_reader data, unsigned contours) const;
    glyph_outline read_compound(font_reader data, std::vector<std::uint16_t>& chain, std::size_t& budget) const;
};
}
