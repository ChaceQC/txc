#pragma once

#include "stdlib/native_gui/core/opentype.hpp"
#include <initializer_list>
#include <stdexcept>

namespace fixture
{
using bytes = std::vector<std::uint8_t>;
inline bytes words(std::initializer_list<unsigned> values)
{
    bytes result;
    for (auto value : values)
    {
        result.push_back(value >> 8);
        result.push_back(value & 255);
    }
    return result;
}
inline void put(bytes& data, std::size_t offset, unsigned value)
{
    data.at(offset) = value >> 8;
    data.at(offset + 1) = value & 255;
}
inline void tag(bytes& data, std::size_t offset, std::uint32_t value)
{
    put(data, offset, value >> 16);
    put(data, offset + 2, value & 65535);
}
inline unsigned add(bytes& data, const bytes& child)
{
    const auto offset = unsigned(data.size());
    data.insert(data.end(), child.begin(), child.end());
    return offset;
}
inline void link(bytes& data, std::size_t offset, const bytes& child)
{
    put(data, offset, add(data, child));
}
inline bytes coverage(std::initializer_list<unsigned> glyphs)
{
    auto result = words({1, unsigned(glyphs.size())});
    add(result, words(glyphs));
    return result;
}
inline bytes lookup(unsigned type, bytes data, unsigned flags = 0)
{
    auto result = words({type, flags, 1, flags & 16 ? 10u : 8u});
    if (flags & 16)
    {
        add(result, words({0}));
    }
    add(result, data);
    return result;
}
inline bytes layout(const std::vector<bytes>& lookups, std::initializer_list<unsigned> active = {0},
    std::string_view feature = "ccmp")
{
    auto result = words({1, 0, 0, 0, 0});
    auto scripts = words({1, 0, 0, 8, 4, 0, 0, 65535, 1, 0});
    tag(scripts, 2, tx::ui::ot_tag("DFLT"));
    link(result, 4, scripts);
    auto features = words({1, 0, 0, 8, 0, unsigned(active.size())});
    tag(features, 2, tx::ui::ot_tag(feature));
    add(features, words(active));
    link(result, 6, features);
    auto list = words({unsigned(lookups.size())});
    list.resize(2 + lookups.size() * 2);
    for (std::size_t i = 0; i < lookups.size(); ++i)
    {
        link(list, 2 + i * 2, lookups[i]);
    }
    link(result, 8, list);
    return result;
}
inline std::vector<tx::ui::shaped_glyph> glyphs(std::initializer_list<unsigned> ids)
{
    std::vector<tx::ui::shaped_glyph> result;
    for (auto id : ids)
    {
        const auto index = result.size();
        result.push_back({static_cast<std::uint16_t>(id), index, index + 1, 500, 0, 0, 1, 0, {}});
    }
    return result;
}
inline void require(bool value, const char* message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}
template<class function>
void rejects(function operation, const char* message)
{
    try
    {
        operation();
    }
    catch (const std::exception&)
    {
        return;
    }
    throw std::runtime_error(message);
}
inline bytes single(unsigned from, unsigned to)
{
    auto result = words({2, 0, 1, to});
    link(result, 2, coverage({from}));
    return lookup(1, result);
}
inline void substitute(const bytes& data, std::vector<tx::ui::shaped_glyph>& glyphs, const bytes& gdef = {})
{
    tx::ui::substitute_glyphs(tx::ui::font_reader(data), tx::ui::font_reader(gdef), 100, glyphs, {});
}
inline void position(const bytes& data, std::vector<tx::ui::shaped_glyph>& glyphs,
    const bytes& gdef = {}, bool rtl = false)
{
    tx::ui::position_glyphs(tx::ui::font_reader(data), tx::ui::font_reader(gdef), 100, glyphs,
        {tx::ui::ot_tag("DFLT"), 0, rtl}, 1, 16);
}
void check_substitution();
void check_positioning();
}
