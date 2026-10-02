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
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    try
    {
        verify_layout(font_face::load(argv[1]));
        std::cout << "native text layout / UAX14 wrap / grapheme emergency / caret / selection PASS\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
