#include "stdlib/native_gui/core/text_layout.hpp"
#include "stdlib/native_gui/core/line_break.hpp"
#include "stdlib/native_gui/core/unicode_tables.hpp"

#include <stdexcept>

namespace tx::ui
{
namespace
{
bool hidden(char32_t scalar)
{
    return hard_line_break(scalar) || scalar == U'\t' || scalar == 0x200d ||
        scalar == 0x200b || scalar == 0x2060 || scalar == 0xfeff || (scalar >= 0xfe00 && scalar <= 0xfe0f) ||
        (scalar >= 0xe0100 && scalar <= 0xe01ef);
}

double cluster_width(const font_face& font, std::u32string_view text, double size)
{
    double width = 0;
    for (const auto scalar : text)
    {
        if (scalar == U'\t')
        {
            width += font.advance(font.glyph(U' '), size) * 4;
        }
        else if (!hidden(scalar) && grapheme_of(scalar) != grapheme_property::extend)
        {
            width += font.advance(font.glyph(scalar), size);
        }
    }
    return width;
}

struct measured_cluster
{
    std::size_t begin, end;
    double advance;
    bool newline, space;
};

std::vector<measured_cluster> measure_clusters(const font_face& font, std::u32string_view text, double size)
{
    const auto boundaries = grapheme_boundaries(text);
    std::vector<measured_cluster> result;
    for (std::size_t index = 0; index + 1 < boundaries.size(); ++index)
    {
        const auto begin = boundaries[index], end = boundaries[index + 1];
        const auto span = text.substr(begin, end - begin);
        const bool newline = hard_line_break(span.front());
        result.push_back({begin, end, newline ? 0 : cluster_width(font, span, size), newline,
            span.size() == 1 && (span.front() == U' ' || span.front() == U'\t')});
    }
    return result;
}

std::vector<std::pair<std::size_t, std::size_t>> wrap_lines(const std::vector<measured_cluster>& clusters,
    const std::vector<line_break_kind>& breaks, double width, bool wrap)
{
    std::vector<std::pair<std::size_t, std::size_t>> result;
    std::size_t start = 0, cursor = 0, opportunity = 0;
    double pen = 0, content_width = 0;
    while (cursor < clusters.size())
    {
        const auto& cluster = clusters[cursor];
        if (cluster.newline)
        {
            result.emplace_back(start, ++cursor);
            start = opportunity = cursor;
            pen = content_width = 0;
            continue;
        }
        if (breaks[cluster.begin] != line_break_kind::prohibited && content_width <= width)
        {
            opportunity = cursor;
        }
        if (wrap && !cluster.space && cursor > start && pen + cluster.advance > width)
        {
            // 行尾普通空格保留索引但不单独占一行；只有无合法机会时才按字素紧急折行。
            const auto end = opportunity > start ? opportunity : cursor;
            result.emplace_back(start, end);
            start = cursor = opportunity = end;
            pen = content_width = 0;
            continue;
        }
        pen += cluster.advance;
        if (!cluster.space)
        {
            content_width = pen;
        }
        ++cursor;
    }
    result.emplace_back(start, clusters.size());
    return result;
}
}

text_layout::text_layout(const font_face& font, std::u32string_view text, double size, double width, bool wrap)
    : font_(font), size_(size), line_height_(font.line_height(size)), length_(text.size())
{
    if (!std::isfinite(width) || width < 0 || width > 1e7)
    {
        throw std::invalid_argument("文本布局宽度非法");
    }
    const auto clusters = measure_clusters(font, text, size);
    const auto breaks = line_breaks(text);
    double y = 0;
    for (const auto& [start, end] : wrap_lines(clusters, breaks, width, wrap))
    {
        double x = 0;
        const auto line_start = start < clusters.size() ? clusters[start].begin : text.size();
        auto line_end = line_start;
        for (auto index = start; index < end; ++index)
        {
            const auto& cluster = clusters[index];
            append(text, cluster.begin, cluster.end, x, y, cluster.advance);
            x += cluster.advance;
            line_end = cluster.newline ? cluster.begin : cluster.end;
        }
        lines_.push_back({line_start, line_end, x, y});
        width_ = std::max(width_, x);
        y += line_height_;
    }
    height_ = y;
}

void text_layout::append(std::u32string_view text, std::size_t begin, std::size_t end,
    double x, double y, double width)
{
    clusters_.push_back({begin, end, {x, y, width, line_height_}});
    double pen = x, base = x;
    for (std::size_t index = begin; index < end; ++index)
    {
        const auto scalar = text[index];
        if (hidden(scalar))
        {
            continue;
        }
        const auto glyph = font_.glyph(scalar);
        const bool mark = grapheme_of(scalar) == grapheme_property::extend;
        glyphs_.push_back({glyph, {mark ? base : pen, y + font_.ascender(size_)}});
        if (!mark)
        {
            base = pen;
            pen += font_.advance(glyph, size_);
        }
    }
}

double text_layout::width() const noexcept
{
    return width_;
}

double text_layout::height() const noexcept
{
    return height_;
}

const std::vector<text_line>& text_layout::lines() const noexcept
{
    return lines_;
}

std::size_t text_layout::hit(point position) const
{
    const auto row = std::clamp(static_cast<int>(std::floor(position.y / line_height_)), 0,
        static_cast<int>(lines_.size() - 1));
    const auto& line = lines_[row];
    for (const auto& cluster : clusters_)
    {
        if (cluster.begin < line.begin || cluster.begin >= line.end)
        {
            continue;
        }
        if (position.x < cluster.bounds.x + cluster.bounds.width / 2)
        {
            return cluster.begin;
        }
        if (position.x < cluster.bounds.x + cluster.bounds.width)
        {
            return cluster.end;
        }
    }
    return position.x < 0 ? line.begin : line.end;
}

rect text_layout::caret(std::size_t index) const
{
    if (index > length_)
    {
        throw std::out_of_range("光标索引超过文本长度");
    }
    for (const auto& cluster : clusters_)
    {
        if (index >= cluster.begin && index < cluster.end)
        {
            return {cluster.bounds.x, cluster.bounds.y, 1, line_height_};
        }
    }
    const auto& line = lines_.back();
    return {line.width, line.y, 1, line_height_};
}

std::vector<rect> text_layout::selection(std::size_t begin, std::size_t end) const
{
    if (begin > end || end > length_)
    {
        throw std::out_of_range("文本选区非法");
    }
    std::vector<rect> result;
    for (const auto& cluster : clusters_)
    {
        if (cluster.end <= begin || cluster.begin >= end)
        {
            continue;
        }
        if (!result.empty() && result.back().y == cluster.bounds.y)
        {
            result.back().width = cluster.bounds.x + std::max(2.0, cluster.bounds.width) - result.back().x;
        }
        else
        {
            auto bounds = cluster.bounds;
            bounds.width = std::max(2.0, bounds.width);
            result.push_back(bounds);
        }
    }
    return result;
}

void text_layout::draw(rasterizer& painter, point origin, color color) const
{
    for (const auto& glyph : glyphs_)
    {
        painter.fill(font_.outline(glyph.glyph, size_,
            {origin.x + glyph.baseline.x, origin.y + glyph.baseline.y}), color);
    }
}
}
