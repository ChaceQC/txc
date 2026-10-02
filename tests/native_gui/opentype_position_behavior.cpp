#include "opentype_fixture.hpp"
#include <cmath>

namespace fixture
{
namespace
{
void values_and_pairs()
{
    auto single = words({1, 0, 7, 10, 20, 65506});
    link(single, 2, coverage({1}));
    auto input = glyphs({1, 2});
    position(layout({lookup(1, single)}, {0}, "kern"), input);
    require(input[0].x == 10 && input[0].y == -20 && input[0].advance == 470, "GPOS value signs and scale");
    auto pair = words({1, 0, 4, 0, 1, 0});
    link(pair, 2, coverage({1}));
    link(pair, 10, words({1, 2, 65496}));
    input = glyphs({1, 2, 1, 3});
    position(layout({lookup(2, pair)}, {0}, "kern"), input);
    require(input[0].advance == 460 && input[2].advance == 500, "GPOS pair format 1");
    auto classified = words({2, 0, 4, 0, 0, 0, 2, 2, 0, 0, 0, 65476});
    link(classified, 2, coverage({1}));
    link(classified, 8, words({1, 1, 1, 1}));
    link(classified, 10, words({1, 2, 1, 1}));
    input = glyphs({1, 2});
    position(layout({lookup(2, classified)}, {0}, "kern"), input);
    require(input[0].advance == 440, "GPOS class pair matrix");
    auto device_pair = words({1, 0, 64, 0, 1, 0});
    link(device_pair, 2, coverage({1}));
    link(device_pair, 10, words({1, 2, 6, 16, 16, 1, 0xc000}));
    input = glyphs({1, 2});
    position(layout({lookup(2, device_pair)}, {0}, "kern"), input);
    require(input[0].advance == 499, "Device delta is signed and relative to PairSet");
    auto extension = words({1, 1, 0, 8});
    add(extension, single);
    input = glyphs({1});
    position(layout({lookup(9, extension)}, {0}, "kern"), input);
    require(input[0].advance == 470, "GPOS extension");
    auto context = words({3, 2, 1, 0, 0, 0, 1});
    link(context, 6, coverage({1}));
    link(context, 8, coverage({2}));
    input = glyphs({1, 2, 1, 3});
    position(layout({lookup(7, context), lookup(1, single)}, {0}, "kern"), input);
    require(input[0].x == 10 && input[2].x == 0, "GPOS contextual positioning");
}

bytes attachment(unsigned type, unsigned mark_glyph, unsigned base_glyph)
{
    auto table = words({1, 0, 0, 1, 0, 0});
    link(table, 2, coverage({mark_glyph}));
    link(table, 4, coverage({base_glyph}));
    link(table, 8, words({1, 0, 6, 1, 10, 20}));
    if (type == 5)
    {
        link(table, 10, words({1, 4, 2, 6, 12, 1, 100, 200, 1, 300, 400}));
    }
    else
    {
        link(table, 10, words({1, 4, 1, 100, 200}));
    }
    return lookup(type, table);
}

void anchors_and_cursive()
{
    auto input = glyphs({1, 3, 4});
    input[1].advance = input[2].advance = 0;
    input[1].glyph_class = input[2].glyph_class = 3;
    position(layout({attachment(4, 3, 1), attachment(6, 4, 3)}, {0, 1}, "mark"), input);
    require(input[1].x == -410 && input[1].y == -180 && input[2].x == -320 && input[2].y == -360,
        "mark-to-base and mark-to-mark share absolute anchor");
    input = glyphs({10, 3});
    input[0].end = 2;
    input[0].components = {0, 1};
    input[1].advance = 0;
    input[1].glyph_class = 3;
    position(layout({attachment(5, 3, 10)}, {0}, "mark"), input);
    require(input[1].x == -210 && input[1].y == -380, "mark selects ligature component");
    input = glyphs({1, 3});
    input[1].advance = 0;
    position(layout({attachment(4, 3, 1)}, {0}, "mark"), input, {}, true);
    require(input[1].x == 90 && input[1].y == -180, "RTL attachment uses RTL glyph origins");
    auto cursive = words({1, 0, 2, 0, 0, 0, 0});
    link(cursive, 2, coverage({1, 2}));
    link(cursive, 8, words({1, 450, 100}));
    link(cursive, 10, words({1, 20, 30}));
    input = glyphs({1, 2});
    position(layout({lookup(3, cursive)}, {0}, "curs"), input);
    require(input[0].advance == 430 && input[1].y == -70, "cursive anchors join");
    input = glyphs({1, 2});
    position(layout({lookup(3, cursive, 1)}, {0}, "curs"), input);
    require(input[0].y == 70 && input[1].y == 0, "cursive right-to-left flag retains final baseline");
    auto chain = words({1, 0, 1, 0, 0});
    link(chain, 2, coverage({1}));
    link(chain, 6, words({1, 20, 30}));
    link(chain, 8, words({1, 450, 100}));
    input.assign(2048, glyphs({1}).front());
    position(layout({lookup(3, chain, 1)}, {0}, "curs"), input);
    require(input.front().y == 70 * 2047 && input.back().y == 0,
        "long cursive chain resolves without recursive stack or quadratic prefix adjustment");
}

void gdef_carets()
{
    auto gdef = words({1, 0, 0, 0, 0, 0});
    auto list = words({0, 1, 0});
    link(list, 0, coverage({10}));
    link(list, 4, words({1, 4, 1, 125}));
    link(gdef, 8, list);
    const auto carets = tx::ui::ligature_carets(tx::ui::font_reader(gdef), 10, 0.5);
    require(carets == std::vector<double>{62.5}, "GDEF ligature caret design coordinate and scale");
    require(tx::ui::ligature_carets(tx::ui::font_reader(gdef), 11, 1).empty(), "uncovered ligature uses caret fallback");
}
}

void check_positioning()
{
    values_and_pairs();
    anchors_and_cursive();
    gdef_carets();
}
}
