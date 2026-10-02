#include "stdlib/native_gui/core/text_layout.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace tx::ui;

namespace
{
void require(bool value, const char* message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}

void verify_layout(const font_face& font)
{
    const auto width = [&](std::u32string_view text)
    {
        return text_layout(font, text, 20, 10000, false).width();
    };
    const text_layout words(font, U"hello world", 20, width(U"hello wo"), true);
    require(words.lines().size() == 2 && words.lines()[0].end == 6 && words.lines()[1].begin == 6,
        "soft wrap must keep words together");
    const text_layout spaces(font, U"hello   world", 20, width(U"hello") + 0.01, true);
    require(spaces.lines()[0].end == 8 && spaces.lines()[1].begin == 8,
        "trailing spaces must stay on the previous line");
    const text_layout punctuation(font, U"甲乙，丙", 20, width(U"甲乙") + 0.01, true);
    require(punctuation.lines().size() == 3 && punctuation.lines()[0].end == 1 &&
        punctuation.lines()[1].begin == 1 && punctuation.lines()[1].end == 3,
        "CJK closing punctuation must not start the next line");
    const text_layout brackets(font, U"甲（乙）丙", 20, width(U"（乙）") + 0.01, true);
    require(brackets.lines()[0].end == 1 && brackets.lines()[1].begin == 1 && brackets.lines()[1].end == 4,
        "CJK opening and closing punctuation must stay with content");
    const std::u32string clusters = U"éé👨‍👩‍👧‍👦🇨🇳x";
    const auto boundaries = grapheme_boundaries(clusters);
    const text_layout emergency(font, clusters, 20, 1, true);
    for (const auto& line : emergency.lines())
    {
        require(std::binary_search(boundaries.begin(), boundaries.end(), line.begin) &&
            std::binary_search(boundaries.begin(), boundaries.end(), line.end), "emergency wrap split a grapheme");
        require(line.end > line.begin, "emergency wrap generated empty line");
    }
    const text_layout hard(font, U"a\r\nb\v\f\x85\u2028\u2029", 20, 10000, false);
    require(hard.lines().size() == 7 && hard.lines()[0].end == 1 && hard.lines()[1].begin == 3 &&
        hard.lines().back().begin == 9, "mandatory breaks / CRLF / final empty line");
    const auto second = words.caret(6);
    require(second.x == 0 && second.y == words.lines()[1].y && words.hit({0, second.y}) == 6,
        "soft-wrap caret and hit must share logical indices");
    require(words.selection(1, 9).size() == 2, "selection crosses two wrapped lines");
    require(text_layout(font, U"hello world", 20, 1, false).lines().size() == 1, "nowrap ignores soft breaks");
    require(text_layout(font, U"", 20, 1, true).lines().size() == 1, "empty text has one line");
}

void verify_bidi_layout(const font_face& font)
{
    const text_layout rtl(font, U"אבג", 20, 10000, false);
    require(rtl.caret(0).x > rtl.caret(1).x && rtl.caret(1).x > rtl.caret(2).x && rtl.caret(3).x == 0,
        "RTL caret must follow logical indices on reversed visual positions");
    require(rtl.hit({-1, 0}) == 3 && rtl.hit({rtl.width() + 1, 0}) == 0,
        "RTL outer hit edges");
    const text_layout mixed(font, U"abc אבג xyz", 20, 10000, false);
    require(mixed.selection(0, 5).size() == 2, "bidi selection must not fill unselected visual gap");
    const text_layout hidden(font, U"a\u2067אב\u2069b", 20, 10000, false);
    const text_layout plain(font, U"aאבb", 20, 10000, false);
    require(std::abs(hidden.width() - plain.width()) < 0.001, "isolate controls have no advance");
    const text_layout paragraphs(font, U"אב\nabc", 20, 10000, false);
    require(paragraphs.caret(3).x == 0 && paragraphs.caret(0).x > paragraphs.caret(1).x,
        "paragraph directions resolve independently");
    const text_layout narrow(font, U"אבג אבג", 20, rtl.width() + 1, true);
    require(narrow.lines().size() == 2 && narrow.caret(4).x > narrow.caret(5).x,
        "soft-wrapped RTL lines preserve resolved levels");
}

void verify_fallback(const font_face& primary, const char* path)
{
    auto fallback = std::make_shared<font_face>(font_face::load(path));
    font_family fonts(primary);
    fonts.add(fallback);
    char32_t scalar = 0;
    for (char32_t candidate = 0x100; candidate < 0xd800; ++candidate)
    {
        if (!primary.glyph(candidate) && fallback->glyph(candidate))
        {
            scalar = candidate;
            break;
        }
    }
    require(scalar != 0, "fallback fixture needs a character absent from primary");
    const std::u32string text(1, scalar);
    require(&fonts.select(text) == fallback.get(), "choose covering fallback face");
    require(&fonts.select(U"abc") == &primary, "retain primary for covered text");
    require(&fonts.select(U"e\u0301") == (primary.glyph(0x301) ? &primary : fallback.get()),
        "combining sequence uses one face");
    const text_layout layout(fonts, text, 20, 10000, false);
    require(std::abs(layout.width() - text_layout(*fallback, text, 20, 10000, false).width()) < 0.001,
        "fallback measurement uses actual face");
    require(layout.caret(1).x == layout.width() && layout.hit({layout.width() - 0.001, 0}) == 1,
        "fallback hit and caret use measured advance");
    pixel_buffer image(100, 100);
    image.clear({255, 255, 255, 255});
    rasterizer painter(image);
    layout.draw(painter, {5, 5}, {0, 0, 0, 255});
    require(std::any_of(image.pixels().begin(), image.pixels().end(), [](auto value)
        {
            return value != 0xffffffff;
        }), "fallback glyph rendered");
}

void verify_visual_carets(const font_face& font)
{
    const text_layout mixed(font, U"abc אבג xyz", 20, 10000, false);
    const auto upstream = mixed.caret({4, caret_affinity::upstream});
    const auto downstream = mixed.caret({4, caret_affinity::downstream});
    require(upstream.x < downstream.x && mixed.alternate_caret({4}).has_value(),
        "bidi boundary has two caret positions");
    const auto hit = mixed.hit_position({upstream.x - 0.01, 0});
    require(hit.index == 4 && hit.affinity == caret_affinity::upstream && mixed.caret(hit).x == upstream.x,
        "pointer hit must retain the clicked side of a bidi boundary");
    auto position = mixed.line_edge({0}, -1);
    std::vector<std::size_t> visited{position.index};
    for (unsigned steps = 0; steps < 30; ++steps)
    {
        const auto next = mixed.move_visual(position, 1);
        if (next == position)
        {
            break;
        }
        require(mixed.caret(next).x > mixed.caret(position).x, "right moves visually right");
        visited.push_back(next.index);
        position = next;
    }
    require(visited == std::vector<std::size_t>({0, 1, 2, 3, 4, 6, 5, 4, 8, 9, 10, 11}),
        "visual traversal must enter and leave RTL runs without jumping or stalling");
    require(mixed.move_visual({4, caret_affinity::downstream}, -1).index == 5,
        "left enters the correct side of an RTL run");
    require(mixed.selection_edge(4, 7, -1).index == 7 && mixed.selection_edge(4, 7, 1).index == 4,
        "selection collapses to its visual edge");
    const text_layout rtl(font, U"אבג", 20, 10000, false);
    require(rtl.line_edge({0}, -1).index == 3 && rtl.line_edge({3}, 1).index == 0,
        "RTL Home/End use visual line edges");
    const text_layout wrapped(font, U"hello world", 20, text_layout(font, U"hello wo", 20, 10000, false).width(), true);
    const auto previous_line = wrapped.caret({6, caret_affinity::upstream});
    const auto next_line = wrapped.caret({6, caret_affinity::downstream});
    require(previous_line.y < next_line.y && wrapped.move_visual({6, caret_affinity::upstream}, 1) ==
        text_position{6, caret_affinity::downstream}, "soft-wrap affinity and visual line transition");
    const text_layout empty(font, U"\n\n", 20, 100, true);
    require(empty.move_visual({0}, 1).index == 1 && empty.move_visual({1}, 1).index == 2,
        "empty lines remain keyboard accessible");
    const text_layout combining(font, U"áב", 20, 100, true);
    require(combining.move_visual({0}, 1).index == 2, "visual movement cannot split a grapheme");
}
}

int main(int argc, char** argv)
{
    if (argc != 2 && argc != 3)
    {
        return 2;
    }
    try
    {
        const auto font = font_face::load(argv[1]);
        verify_layout(font);
        verify_bidi_layout(font);
        verify_visual_carets(font);
        if (argc == 3)
        {
            verify_fallback(font, argv[2]);
        }
        std::cout << "native text layout / UAX14 / bidi / caret / selection PASS; fallback "
            << (argc == 3 ? "PASS" : "not requested") << '\n';
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
