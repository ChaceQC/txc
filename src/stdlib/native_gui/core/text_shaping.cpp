#include "stdlib/native_gui/core/text_shaping.hpp"
#include "stdlib/native_gui/core/opentype.hpp"
#include "stdlib/native_gui/core/bidi.hpp"
#include "stdlib/native_gui/core/line_break.hpp"
#include <algorithm>

namespace tx::ui
{
std::vector<layout_cluster> text_clusters(const font_family& fonts, std::u32string_view text,
    const std::vector<unsigned>& levels)
{
    const auto boundaries = grapheme_boundaries(text);
    std::vector<layout_cluster> result;
    std::uint32_t previous_script = 0;
    for (std::size_t i = 0; i + 1 < boundaries.size(); ++i)
    {
        const auto begin = boundaries[i], end = boundaries[i + 1];
        auto span = std::u32string(text.substr(begin, end - begin));
        std::uint32_t script = 0;
        for (auto& scalar : span)
        {
            if (!script)
            {
                script = shaping_script(scalar);
            }
            if (levels[begin] % 2)
            {
                scalar = bidi_mirror(scalar);
            }
        }
        const bool newline = hard_line_break(span.front());
        result.push_back({begin, end, 0, newline, span.size() == 1 && (span.front() == U' ' || span.front() == U'\t'),
            &fonts.select(span), script ? script : previous_script, {}});
        previous_script = newline ? 0 : result.back().script;
    }
    std::uint32_t following_script = 0;
    for (auto i = result.size(); i > 0; --i)
    {
        auto& cluster = result[i - 1];
        if (cluster.newline)
        {
            following_script = 0;
        }
        else if (!cluster.script)
        {
            cluster.script = following_script;
        }
        else
        {
            following_script = cluster.script;
        }
    }
    return result;
}

namespace
{
void shape_run(std::vector<layout_cluster>& clusters, std::u32string_view text,
    std::size_t start, std::size_t end, double size, bool rtl)
{
    const auto begin = clusters[start].begin, limit = clusters[end - 1].end;
    const auto& face = *clusters[start].face;
    const auto glyphs = shape_text(face, text.substr(begin, limit - begin), size, rtl);
    std::vector<std::pair<std::size_t, std::size_t>> spans;
    for (const auto& glyph : glyphs)
    {
        const auto first = std::lower_bound(clusters.begin() + start, clusters.begin() + end, glyph.begin + begin,
            [](const auto& cluster, auto value)
            {
                return cluster.end <= value;
            });
        const auto last = std::lower_bound(first, clusters.begin() + end, glyph.end + begin,
            [](const auto& cluster, auto value)
            {
                return cluster.begin < value;
            });
        if (first == last)
        {
            throw std::invalid_argument("整形字形丢失逻辑字素区间");
        }
        spans.emplace_back(first - clusters.begin(), last - clusters.begin());
        auto carets = glyph.components.empty() ? std::vector<double>{} :
            ligature_carets(face.layout_table("GDEF"), glyph.glyph, face.em_scale(size));
        const auto count = std::size_t(last - first);
        if (carets.size() + 1 != count || !std::is_sorted(carets.begin(), carets.end()) ||
            (!carets.empty() && (carets.front() <= 0 || carets.back() >= glyph.advance)))
        {
            carets.clear();
            for (std::size_t i = 1; i < count; ++i)
            {
                carets.push_back(glyph.advance * double(i) / double(count));
            }
        }
        carets.insert(carets.begin(), 0);
        carets.push_back(glyph.advance);
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto visual = rtl ? count - i - 1 : i;
            (first + i)->advance += carets[visual + 1] - carets[visual];
        }
    }
    double total = 0;
    std::vector<double> positions;
    for (auto i = start; i < end; ++i)
    {
        positions.push_back(total);
        total += clusters[i].advance;
    }
    double pen = 0;
    for (std::size_t i = 0; i < glyphs.size(); ++i)
    {
        const auto& glyph = glyphs[i];
        auto& cluster = clusters[spans[i].first];
        const auto cluster_x = rtl ? total - positions[spans[i].first - start] - cluster.advance : positions[spans[i].first - start];
        const auto glyph_x = rtl ? total - pen - glyph.advance : pen;
        if (!glyph.invisible)
        {
            cluster.glyphs.push_back({glyph.glyph, {glyph_x + glyph.x - cluster_x, glyph.y}, &face});
        }
        pen += glyph.advance;
    }
}
}

void shape_clusters(std::vector<layout_cluster>& clusters, std::u32string_view text,
    std::size_t start, std::size_t end, const std::vector<unsigned>& levels, std::size_t offset, double size)
{
    for (auto i = start; i < end; ++i)
    {
        clusters[i].advance = 0;
        clusters[i].glyphs.clear();
    }
    while (start < end)
    {
        auto& cluster = clusters[start];
        if (cluster.newline || text[cluster.begin] == U'\t')
        {
            cluster.advance = cluster.newline ? 0 : cluster.face->advance(cluster.face->glyph(U' '), size) * 4;
            ++start;
            continue;
        }
        auto stop = start + 1;
        const auto level = levels[cluster.begin - offset];
        while (stop < end && !clusters[stop].newline && text[clusters[stop].begin] != U'\t' &&
            clusters[stop].face == cluster.face && clusters[stop].script == cluster.script &&
            levels[clusters[stop].begin - offset] == level)
        {
            ++stop;
        }
        shape_run(clusters, text, start, stop, size, level % 2 != 0);
        start = stop;
    }
}
}
