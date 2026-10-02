#include "stdlib/native_gui/core/font_family.hpp"
#include "stdlib/native_gui/core/bidi.hpp"
#include "stdlib/native_gui/core/line_break.hpp"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace tx::ui
{
namespace
{
bool needs_glyph(char32_t scalar)
{
    return !bidi_invisible(scalar) && !hard_line_break(scalar) && scalar != U'\t' &&
        scalar != 0x200d && !(scalar >= 0xfe00 && scalar <= 0xfe0f) &&
        !(scalar >= 0xe0100 && scalar <= 0xe01ef);
}
}

font_family::font_family(const font_face& primary) : faces_{&primary}
{
}

font_family::font_family(std::shared_ptr<const font_face> primary)
{
    add(std::move(primary));
}

void font_family::add(std::shared_ptr<const font_face> face)
{
    if (!face || faces_.size() >= 32)
    {
        throw std::invalid_argument("字体集合要求有效字体且不得超过 32 个");
    }
    // 先接管所有权，再记录地址；拷贝集合时布局继续持有字体。
    owned_.push_back(std::move(face));
    faces_.push_back(owned_.back().get());
}

const font_face& font_family::primary() const
{
    return *faces_.front();
}

const font_face& font_family::select(std::u32string_view cluster) const
{
    for (const auto* face : faces_)
    {
        if (std::all_of(cluster.begin(), cluster.end(), [&](char32_t scalar)
            {
                return !needs_glyph(scalar) || face->glyph(scalar) != 0;
            }))
        {
            return *face;
        }
    }
    // 没有覆盖整个字素的字体时保留主字体的 .notdef，不拆散组合序列拼凑字体。
    return primary();
}

double font_family::line_height(double size) const
{
    double below_baseline = 0;
    for (const auto* face : faces_)
    {
        below_baseline = std::max(below_baseline, face->line_height(size) - face->ascender(size));
    }
    return ascender(size) + below_baseline;
}

double font_family::ascender(double size) const
{
    double result = 0;
    for (const auto* face : faces_)
    {
        result = std::max(result, face->ascender(size));
    }
    return result;
}

font_family system_font_family(const std::filesystem::path& primary)
{
    font_family result(std::make_shared<font_face>(font_face::load(primary)));
    std::vector<std::filesystem::path> candidates;
#ifdef _WIN32
    const auto* directory = std::getenv("SystemRoot");
    const auto fonts = std::filesystem::path(directory ? directory : "C:/Windows") / "Fonts";
    for (const auto* name : {"segoeui.ttf", "seguisym.ttf", "msyh.ttc", "arial.ttf", "simsun.ttc"})
    {
        candidates.push_back(fonts / name);
    }
#else
    for (const auto* name : {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSansArabic-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSansDevanagari-Regular.ttf"})
    {
        candidates.emplace_back(name);
    }
#endif
    for (const auto& candidate : candidates)
    {
        std::error_code error;
        if (!std::filesystem::is_regular_file(candidate, error) || std::filesystem::equivalent(primary, candidate, error))
        {
            continue;
        }
        try
        {
            result.add(std::make_shared<font_face>(font_face::load(candidate)));
        }
        catch (const std::invalid_argument&)
        {
            // 自动候选的损坏或不支持格式不阻止主字体使用；显式主字体错误仍向上传递。
        }
    }
    return result;
}
}
