#include "stdlib/native_gui/core/opentype_internal.hpp"
#include <algorithm>
#include <set>

namespace tx::ui::ot
{
processor::processor(font_reader table, font_reader gdef, unsigned glyph_count,
    std::vector<shaped_glyph>& input, shaping_options settings, bool positioning, double scale, double size)
    : glyphs(input), options(settings), scale(scale), size(size), table_(table), gdef_(gdef),
      lookup_list_({}), glyph_count_(glyph_count), positioning_(positioning),
      budget_(std::min<std::size_t>(10000000, input.size() * 256 + 65536))
{
    if (input.size() > 1000000 || !glyph_count || glyph_count > 65536 ||
        !std::isfinite(scale) || scale <= 0 || !std::isfinite(size) || size <= 0 || size > 4096)
    {
        throw std::invalid_argument("OpenType 整形输入超过限制");
    }
    if (gdef.size() && gdef.u32(0) != 0x00010000 && gdef.u32(0) != 0x00010002 &&
        gdef.u32(0) != 0x00010003)
    {
        throw std::invalid_argument("GDEF 版本非法");
    }
    for (const auto& glyph : glyphs)
    {
        check_glyph(glyph.glyph);
    }
    if (positioning)
    {
        cursive_parents_.resize(glyphs.size(), glyphs.size());
        cursive_deltas_.resize(glyphs.size());
    }
}

void processor::spend()
{
    if (!budget_--)
    {
        throw std::length_error("OpenType 整形操作超过预算");
    }
}

void processor::check_glyph(unsigned glyph) const
{
    if (glyph >= glyph_count_)
    {
        throw std::invalid_argument("OpenType 输出字形编号越界");
    }
}

lookup processor::get_lookup(unsigned index) const
{
    if (index >= lookup_list_.u16(0))
    {
        throw std::invalid_argument("OpenType Lookup 索引越界");
    }
    const auto data = subtable(lookup_list_, lookup_list_.u16(2 + index * 2));
    const auto count = data.u16(4), flags = data.u16(2), type = data.u16(0);
    data.slice(6, std::size_t(count) * 2);
    if (!type || type > (positioning_ ? 9 : 8) || (flags & 0xe0))
    {
        throw std::invalid_argument("OpenType Lookup 类型或标志非法");
    }
    return {data, type, flags, flags & 16 ? data.u16(6 + count * 2) : 0u};
}

application processor::apply(unsigned index, std::size_t position, unsigned depth)
{
    spend();
    if (depth >= 16)
    {
        throw std::length_error("OpenType 上下文 Lookup 递归过深");
    }
    const auto entry = get_lookup(index);
    if (position >= glyphs.size() || ignored(entry, position))
    {
        return {};
    }
    for (unsigned i = 0; i < entry.data.u16(4); ++i)
    {
        spend();
        auto data = subtable(entry.data, entry.data.u16(6 + i * 2));
        auto type = entry.type;
        const unsigned extension = positioning_ ? 9 : 7;
        if (type == extension)
        {
            type = data.u16(2);
            if (data.u16(0) != 1 || !type || type == extension || type > 8)
            {
                throw std::invalid_argument("OpenType Extension Lookup 格式非法");
            }
            data = subtable(data, data.u32(4));
        }
        const auto result = positioning_ ? positioning(entry, data, type, position, depth) :
            substitution(entry, data, type, position, depth);
        if (result.matched)
        {
            return result;
        }
    }
    return {};
}

void processor::feature(unsigned index, std::uint32_t tag)
{
    const auto features = subtable(table_, table_.u16(6));
    if (index >= features.u16(0))
    {
        throw std::invalid_argument("OpenType Feature 索引越界");
    }
    const auto data = subtable(features, features.u16(6 + index * 6));
    data.slice(4, std::size_t(data.u16(2)) * 2);
    std::set<unsigned> ordered;
    for (unsigned i = 0; i < data.u16(2); ++i)
    {
        ordered.insert(data.u16(4 + i * 2));
    }
    const bool form = tag == ot_tag("isol") || tag == ot_tag("init") ||
        tag == ot_tag("medi") || tag == ot_tag("fina");
    for (auto lookup_index : ordered)
    {
        const auto entry = get_lookup(lookup_index);
        bool reverse = !positioning_ && entry.type == 8;
        if (!positioning_ && entry.type == 7 && entry.data.u16(4))
        {
            reverse = subtable(entry.data, entry.data.u16(6)).u16(2) == 8;
        }
        if (reverse)
        {
            for (auto i = glyphs.size(); i > 0; --i)
            {
                if (!form || glyphs[i - 1].form == tag)
                {
                    apply(lookup_index, i - 1, 0);
                }
            }
            continue;
        }
        for (std::size_t i = 0; i < glyphs.size();)
        {
            if (form && glyphs[i].form != tag)
            {
                ++i;
                continue;
            }
            const auto result = apply(lookup_index, i, 0);
            i = result.matched ? result.next : i + 1;
        }
        finish_cursive();
    }
}

namespace
{
unsigned tagged_offset(font_reader list, std::uint32_t tag)
{
    const auto count = list.u16(0);
    list.slice(2, std::size_t(count) * 6);
    for (unsigned i = 0; i < count; ++i)
    {
        if (list.u32(2 + i * 6) == tag)
        {
            return list.u16(6 + i * 6);
        }
    }
    return 0;
}
}

void processor::run()
{
    if (!table_.size() || glyphs.empty())
    {
        return;
    }
    if (table_.u32(0) != 0x00010000 && table_.u32(0) != 0x00010001)
    {
        throw std::invalid_argument("OpenType Layout 版本非法");
    }
    lookup_list_ = subtable(table_, table_.u16(8));
    lookup_list_.slice(2, std::size_t(lookup_list_.u16(0)) * 2);
    const auto scripts = subtable(table_, table_.u16(4));
    auto offset = tagged_offset(scripts, options.script);
    if (!offset)
    {
        offset = tagged_offset(scripts, ot_tag("DFLT"));
    }
    if (!offset)
    {
        return;
    }
    const auto script = subtable(scripts, offset);
    offset = options.language ? tagged_offset(subtable(script, 2), options.language) : 0;
    if (!offset)
    {
        offset = script.u16(0);
    }
    if (!offset)
    {
        return;
    }
    const auto language = subtable(script, offset), features = subtable(table_, table_.u16(6));
    language.slice(6, std::size_t(language.u16(4)) * 2);
    const auto required = language.u16(2);
    if (required != 0xffff)
    {
        feature(required, 0);
    }
    // 字体查找依语言系统选择；连接形态在 ccmp/locl 后、连字前应用。
    const std::vector<std::string_view> stages = positioning_ ?
        std::vector<std::string_view>{"curs", "kern", "dist", "abvm", "blwm", "mark", "mkmk"} :
        std::vector<std::string_view>{"ccmp", "locl", "isol", "fina", "medi", "init", "rlig", "calt", "liga", "clig"};
    for (const auto stage : stages)
    {
        for (unsigned i = 0; i < language.u16(4); ++i)
        {
            const auto index = language.u16(6 + i * 2);
            if (index >= features.u16(0))
            {
                throw std::invalid_argument("LangSys 特性索引越界");
            }
            if (index != required && features.u32(2 + index * 6) == ot_tag(stage))
            {
                feature(index, ot_tag(stage));
            }
        }
    }
}
}

namespace tx::ui
{
void substitute_glyphs(font_reader gsub, font_reader gdef, unsigned count,
    std::vector<shaped_glyph>& glyphs, shaping_options options)
{
    ot::processor(gsub, gdef, count, glyphs, options, false).run();
}

void position_glyphs(font_reader gpos, font_reader gdef, unsigned count,
    std::vector<shaped_glyph>& glyphs, shaping_options options, double scale, double size)
{
    ot::processor(gpos, gdef, count, glyphs, options, true, scale, size).run();
}
}
