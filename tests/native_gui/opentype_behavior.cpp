#include "opentype_fixture.hpp"
#include "stdlib/native_gui/core/text_layout.hpp"
#include "stdlib/native_gui/core/image_io.hpp"
#include <iostream>
#include <numeric>

using namespace tx::ui;
using fixture::require;

namespace
{
void check_joining()
{
    require(shaping_script(U'あ') == ot_tag("kana") && shaping_script(U'ア') == ot_tag("kana"),
        "Unicode Hiragana and Katakana share the OpenType kana script tag");
    require(arabic_forms(U"ببب") == std::vector<std::uint32_t>{ot_tag("init"), ot_tag("medi"), ot_tag("fina")},
        "Arabic initial medial final");
    require(arabic_forms(U"بَب")[2] == ot_tag("fina"), "transparent Arabic mark does not break joining");
    require(arabic_forms(U"ب\u200cب")[0] == ot_tag("isol") &&
        arabic_forms(U"ب\u200d")[0] == ot_tag("init"), "ZWNJ prevents / ZWJ causes joining");
    require(arabic_forms(U"اب")[0] == ot_tag("isol") && arabic_forms(U"اب")[1] == ot_tag("isol"),
        "right-joining alef cannot join to following beh");
}

void check_real_font(const char* path, const char* output)
{
    const auto font = font_face::load(path);
    const auto latin = shape_text(font, U"office AV", 32, false);
    const auto arabic = shape_text(font, U"سلام", 32, true);
    require(arabic.size() < 4, "real font must substitute lam-alef ligature");
    require(arabic[0].glyph != font.glyph(U'س'), "real Arabic glyph must receive joining form");
    const auto marks = shape_text(font, U"بَ", 32, true);
    require(marks.size() == 2 && marks[1].advance == 0 && marks[1].y != 0,
        "Arabic mark must receive GPOS anchor positioning");
    const auto separated = shape_text(font, U"ل\u200cا", 32, true);
    require(separated.size() == 3 && separated[1].invisible, "ZWNJ breaks lam-alef ligature without rendering");
    const auto width = std::accumulate(latin.begin(), latin.end(), 0.0, [](double total, const auto& glyph)
        {
            return total + glyph.advance;
        });
    const text_layout layout(font, U"office AV", 32, 1000, false);
    require(std::abs(width - layout.width()) < 0.001, "layout must use shaping advance");
    const text_layout ligature(font, U"لا", 32, 1000, false);
    require(ligature.caret(0).x > ligature.caret(1).x && ligature.caret(1).x > ligature.caret(2).x,
        "ligature keeps interior logical caret and RTL navigation");
    require(ligature.move_visual({0}, -1).index == 1 && ligature.move_visual({1}, -1).index == 2,
        "ligature visual navigation visits each original grapheme");
    const text_layout wrapped(font, U"سلامسلامسلام", 32, 45, true);
    require(wrapped.lines().size() > 1, "Arabic text wraps");
    const std::u32string original = U"سلامسلامسلام";
    for (const auto& line : wrapped.lines())
    {
        const text_layout isolated(font, original.substr(line.begin, line.end - line.begin), 32, 1000, false);
        require(std::abs(isolated.width() - line.width) < 0.001, "soft line must reshape at its boundaries");
    }
    pixel_buffer image(1000, 300);
    image.clear({245, 247, 250, 255});
    rasterizer painter(image);
    text_layout(font, U"OpenType: office  ffi  AVATAR\nالعربية: السَّلَامُ عَلَيْكُمْ\nسلام  لا  ل\u200cا  بَب\nabc אבג العربية 123", 36, 950, true)
        .draw(painter, {24, 24}, {28, 42, 61, 255});
    save_bitmap(image, output);
    std::cout << "real font: Latin glyphs=" << latin.size() << ", Arabic glyphs=" << arabic.size() << '\n';
}
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        return 2;
    }
    try
    {
        fixture::check_substitution();
        fixture::check_positioning();
        check_joining();
        check_real_font(argv[1], argv[2]);
        std::cout << "OpenType GSUB / GPOS / GDEF / Arabic / shaping-layout PASS\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
