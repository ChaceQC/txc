#include "stdlib/native_gui/core/text_layout.hpp"
#include "stdlib/native_gui/core/line_break.hpp"
#include "stdlib/native_gui/core/unicode_tables.hpp"
#include "stdlib/native_gui/core/bidi.hpp"
#include "stdlib/native_gui/core/text_shaping.hpp"
#include <numeric>

#include <stdexcept>

namespace tx::ui
{
namespace
{
struct paragraph_layout
{
    bidi_paragraph bidi;
    std::vector<unsigned> bases;
};

paragraph_layout paragraph_levels(std::u32string_view text)
{
    paragraph_layout result;
    for (std::size_t start = 0; start < text.size();)
    {
        auto end = start;
        while (end < text.size() && bidi_property(text[end]) != bidi_type::b)
        {
            ++end;
        }
        if (end < text.size())
        {
            ++end;
            if (text[end - 1] == U'\r' && end < text.size() && text[end] == U'\n')
            {
                ++end;
            }
        }
        auto paragraph = resolve_bidi(text.substr(start, end - start));
        result.bidi.original.insert(result.bidi.original.end(), paragraph.original.begin(), paragraph.original.end());
        result.bidi.levels.insert(result.bidi.levels.end(), paragraph.levels.begin(), paragraph.levels.end());
        result.bases.insert(result.bases.end(), end - start, paragraph.base);
        start = end;
    }
    result.bases.push_back(0);
    return result;
}

std::vector<std::size_t> cluster_order(const std::vector<layout_cluster>& clusters,
    std::size_t start, std::size_t end, const std::vector<unsigned>& levels, std::size_t offset)
{
    std::vector<std::size_t> result(end - start);
    std::iota(result.begin(), result.end(), start);
    unsigned maximum = 0;
    for (auto index : result)
    {
        maximum = std::max(maximum, levels[clusters[index].begin - offset]);
    }
    // UAX #9 L3：字素内部保持逻辑顺序，组合标记随基字符移动。
    for (auto level = maximum; level > 0; --level)
    {
        for (std::size_t i = 0; i < result.size();)
        {
            if (levels[clusters[result[i]].begin - offset] < level)
            {
                ++i;
                continue;
            }
            auto stop = i + 1;
            while (stop < result.size() && levels[clusters[result[stop]].begin - offset] >= level)
            {
                ++stop;
            }
            std::reverse(result.begin() + i, result.begin() + stop);
            i = stop;
        }
    }
    return result;
}

std::vector<std::pair<std::size_t, std::size_t>> wrap_lines(const std::vector<layout_cluster>& clusters,
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
    : text_layout(font_family(font), text, size, width, wrap)
{
}

text_layout::text_layout(const font_family& fonts, std::u32string_view text, double size, double width, bool wrap)
    : fonts_(fonts), size_(size), line_height_(fonts.line_height(size)), length_(text.size())
{
    if (!std::isfinite(width) || width < 0 || width > 1e7)
    {
        throw std::invalid_argument("文本布局宽度非法");
    }
    auto paragraphs = paragraph_levels(text);
    auto clusters = text_clusters(fonts_, text, paragraphs.bidi.levels);
    shape_clusters(clusters, text, 0, clusters.size(), paragraphs.bidi.levels, 0, size);
    const auto breaks = line_breaks(text);
    double y = 0;
    auto ranges = wrap_lines(clusters, breaks, width, wrap);
    for (std::size_t row = 0; row < ranges.size(); ++row)
    {
        const auto [start, end] = ranges[row];
        double x = 0;
        const auto line_start = start < clusters.size() ? clusters[start].begin : text.size();
        const auto line_end = end > start ? (clusters[end - 1].newline ? clusters[end - 1].begin : clusters[end - 1].end) : line_start;
        const auto scalar_end = end > start ? clusters[end - 1].end : line_start;
        paragraphs.bidi.base = paragraphs.bases[line_start];
        auto levels = paragraphs.bidi.line_levels(line_start, scalar_end);
        for (auto i = line_start; i < scalar_end; ++i)
        {
            if (bidi_removed(paragraphs.bidi.original[i]))
            {
                levels[i - line_start] = i > line_start ? levels[i - line_start - 1] : paragraphs.bidi.base;
            }
        }
        // 行边界改变连接形态与上下文查找；不能复用跨越软换行的段落连字。
        shape_clusters(clusters, text, start, end, levels, line_start, size);
        if (wrap && end > start + 1)
        {
            const auto refined = wrap_lines(std::vector<layout_cluster>(clusters.begin() + start, clusters.begin() + end),
                breaks, width, true);
            const auto first_end = start + refined.front().second;
            if (first_end < end)
            {
                ranges[row].second = first_end;
                ranges.insert(ranges.begin() + row + 1, {first_end, end});
                --row;
                continue;
            }
        }
        for (auto index : cluster_order(clusters, start, end, levels, line_start))
        {
            const auto& cluster = clusters[index];
            clusters_.push_back({cluster.begin, cluster.end, {x, y, cluster.advance, line_height_},
                levels[cluster.begin - line_start] % 2 != 0});
            for (auto glyph : cluster.glyphs)
            {
                glyph.baseline.x += x;
                glyph.baseline.y += y + fonts_.ascender(size_);
                glyphs_.push_back(glyph);
            }
            x += cluster.advance;
        }
        lines_.push_back({line_start, line_end, x, y});
        width_ = std::max(width_, x);
        y += line_height_;
    }
    height_ = y;
    build_carets();
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
    return hit_position(position).index;
}

rect text_layout::caret(std::size_t index) const
{
    return caret(text_position{index});
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
        if (!result.empty() && result.back().y == cluster.bounds.y &&
            std::abs(result.back().x + result.back().width - cluster.bounds.x) < 0.01)
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
        painter.fill(glyph.face->outline(glyph.glyph, size_,
            {origin.x + glyph.baseline.x, origin.y + glyph.baseline.y}), color);
    }
}
}
