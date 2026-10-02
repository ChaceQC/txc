#pragma once

#include "stdlib/native_gui/core/opentype.hpp"
#include <optional>
#include <stdexcept>

namespace tx::ui::ot
{
font_reader subtable(font_reader data, std::size_t offset);
int coverage(font_reader data, std::uint16_t glyph);
unsigned class_of(font_reader data, std::uint16_t glyph);
struct lookup
{
    font_reader data;
    unsigned type = 0, flags = 0, mark_set = 0;
};
struct application
{
    bool matched = false;
    std::size_t next = 0;
};

class processor
{
public:
    processor(font_reader table, font_reader gdef, unsigned glyph_count,
        std::vector<shaped_glyph>& glyphs, shaping_options options, bool positioning,
        double scale = 1, double size = 1);
    void run();
    application apply(unsigned index, std::size_t position, unsigned depth);
    unsigned glyph_class(const shaped_glyph& glyph) const;
    bool ignored(const lookup& entry, std::size_t position) const;
    std::optional<std::size_t> next(const lookup& entry, std::size_t position, int direction = 1);
    void check_glyph(unsigned glyph) const;
    void spend();
    application substitution(const lookup& entry, font_reader data, unsigned type,
        std::size_t position, unsigned depth);
    application positioning(const lookup& entry, font_reader data, unsigned type,
        std::size_t position, unsigned depth);
    application context(const lookup& entry, font_reader data, bool chained,
        std::size_t position, unsigned depth);
    application attach(const lookup& entry, font_reader data, unsigned type, std::size_t position);
    double device(font_reader data) const;
    std::optional<point> anchor(font_reader parent, unsigned offset) const;
    void value(font_reader data, std::size_t offset, unsigned format, shaped_glyph& glyph) const;
    double origin(std::size_t from, std::size_t to) const;
    void cursive_link(std::size_t child, std::size_t parent, double delta);
    std::vector<shaped_glyph>& glyphs;
    shaping_options options;
    double scale, size;
private:
    font_reader table_, gdef_, lookup_list_;
    unsigned glyph_count_;
    bool positioning_;
    std::size_t budget_;
    std::vector<std::size_t> cursive_parents_;
    std::vector<double> cursive_deltas_;
    void finish_cursive();
    lookup get_lookup(unsigned index) const;
    void feature(unsigned index, std::uint32_t tag);
};
unsigned value_size(unsigned format);
}
