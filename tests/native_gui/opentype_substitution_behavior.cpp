#include "opentype_fixture.hpp"

namespace fixture
{
namespace
{
void basic_substitution()
{
    auto input = glyphs({1, 2});
    const auto replacement = layout({single(1, 10)});
    substitute(replacement, input);
    require(input[0].glyph == 10 && input[1].glyph == 2, "GSUB single / feature selection");
    auto delta = words({1, 0, 65535});
    link(delta, 2, words({2, 1, 2, 3, 0}));
    input = glyphs({2, 3});
    substitute(layout({lookup(1, delta)}), input);
    require(input[0].glyph == 1 && input[1].glyph == 2, "GSUB delta / coverage range");
    auto sequence = words({1, 0, 1, 0});
    link(sequence, 2, coverage({1}));
    link(sequence, 6, words({2, 10, 11}));
    input = glyphs({1, 2});
    substitute(layout({lookup(2, sequence)}), input);
    require(input.size() == 3 && input[0].glyph == 10 && input[1].glyph == 11 && input[1].begin == 0,
        "GSUB multiple must preserve source span");
    input = glyphs({1});
    substitute(layout({lookup(3, sequence)}), input);
    require(input.size() == 1 && input[0].glyph == 10, "GSUB alternate default is first alternative");
    auto extension = words({1, 1, 0, 8});
    add(extension, delta);
    input = glyphs({3});
    substitute(layout({lookup(7, extension)}), input);
    require(input[0].glyph == 2, "GSUB extension offset");
    rejects([&]
        {
            auto invalid = glyphs({1});
            substitute(layout({single(1, 101)}), invalid);
        }, "GSUB glyph range must be checked");
    for (std::size_t length = 1; length < replacement.size(); ++length)
    {
        rejects([&]
            {
                auto invalid = glyphs({1});
                substitute(bytes(replacement.begin(), replacement.begin() + length), invalid);
            }, "GSUB truncated tables must reject");
    }
}

bytes ligature_lookup(unsigned flags = 8)
{
    auto ligature = words({1, 0, 1, 0});
    link(ligature, 2, coverage({1}));
    link(ligature, 6, words({1, 4, 10, 2, 2}));
    return lookup(4, ligature, flags);
}

void ligatures_and_context()
{
    auto gdef = words({1, 0, 0, 0, 0, 0});
    link(gdef, 4, words({2, 3, 1, 2, 1, 3, 3, 3, 10, 10, 2}));
    auto input = glyphs({1, 3, 2});
    substitute(layout({ligature_lookup()}), input, gdef);
    require(input.size() == 2 && input[0].glyph == 10 && input[1].glyph == 3 &&
        input[0].begin == 0 && input[0].end == 3 && input[0].components.size() == 2,
        "GSUB ligature skips GDEF marks and retains components");
    auto context = words({3, 3, 2, 0, 0, 0, 0, 1, 1, 2});
    link(context, 6, coverage({1}));
    link(context, 8, coverage({2}));
    link(context, 10, coverage({4}));
    input = glyphs({1, 2, 4});
    substitute(layout({lookup(5, context), ligature_lookup(0), single(4, 11)}), input);
    require(input.size() == 2 && input[0].glyph == 10 && input[1].glyph == 11,
        "GSUB contextual sequence indices refer to modified sequence");
    auto chained = words({3, 1, 0, 1, 0, 1, 0, 1, 0, 1});
    link(chained, 4, coverage({1}));
    link(chained, 8, coverage({2}));
    link(chained, 12, coverage({3}));
    input = glyphs({1, 2, 3, 2});
    substitute(layout({lookup(6, chained), single(2, 12)}), input);
    require(input[1].glyph == 12 && input[3].glyph == 2, "GSUB chained context before/input/after");
    input = glyphs({4, 2, 3});
    substitute(layout({lookup(6, chained), single(2, 12)}), input);
    require(input[1].glyph == 2, "GSUB chained context rejects wrong backtrack");
    auto simple = words({1, 0, 1, 0});
    link(simple, 2, coverage({1}));
    link(simple, 6, words({1, 4, 2, 1, 2, 1, 1}));
    input = glyphs({1, 2});
    substitute(layout({lookup(5, simple), single(2, 12)}), input);
    require(input[1].glyph == 12, "GSUB context format 1");
    auto classified = words({2, 0, 0, 2, 0, 0});
    link(classified, 2, coverage({1}));
    link(classified, 4,  words({1, 1, 2, 1, 2}));
    link(classified, 10, words({1, 4, 2, 1, 2, 1, 1}));
    input = glyphs({1, 2});
    substitute(layout({lookup(5, classified), single(2, 12)}), input);
    require(input[1].glyph == 12, "GSUB context format 2");
    put(context, 14, 0);
    rejects([&]
        {
            auto invalid = glyphs({1, 2, 4});
            substitute(layout({lookup(5, context)}), invalid);
        }, "cyclic contextual lookups must hit recursion bound");
}

void reverse_and_filter()
{
    auto reverse = words({1, 0, 1, 0, 0, 1, 10});
    link(reverse, 2, coverage({1}));
    link(reverse, 6, coverage({1}));
    auto input = glyphs({1, 1, 1});
    substitute(layout({lookup(8, reverse)}), input);
    require(input[0].glyph == 1 && input[1].glyph == 10 && input[2].glyph == 10,
        "reverse chaining must visit end before start");
    auto gdef = words({1, 2, 0, 0, 0, 0, 0});
    link(gdef, 4, words({1, 1, 3, 1, 1, 3}));
    auto sets = words({1, 1, 0, 8});
    add(sets, coverage({3}));
    link(gdef, 12, sets);
    input = glyphs({1, 3, 2});
    substitute(layout({ligature_lookup(16)}), input, gdef);
    require(input.size() == 3, "marks inside filtering set participate in matching");
    sets = words({1, 1, 0, 8});
    add(sets, coverage({4}));
    link(gdef, 12, sets);
    substitute(layout({ligature_lookup(16)}), input, gdef);
    require(input.size() == 2 && input[0].glyph == 10, "marks outside filtering set are skipped");
}

void chained_formats_and_deletion()
{
    auto rule = words({1, 1, 1, 1, 3, 1, 0, 1});
    auto simple = words({1, 0, 1, 0});
    link(simple, 2, coverage({2}));
    auto set = words({1, 4});
    add(set, rule);
    link(simple, 6, set);
    auto input = glyphs({1, 2, 3});
    substitute(layout({lookup(6, simple), single(2, 12)}), input);
    require(input[1].glyph == 12, "GSUB chained format 1");
    auto classified = words({2, 0, 0, 0, 0, 2, 0, 0});
    link(classified, 2, coverage({2}));
    link(classified, 4, words({1, 1, 1, 1}));
    link(classified, 6, words({1, 2, 1, 1}));
    link(classified, 8, words({1, 3, 1, 3}));
    link(classified, 14, set);
    input = glyphs({1, 2, 3});
    substitute(layout({lookup(6, classified), single(2, 12)}), input);
    require(input[1].glyph == 12, "GSUB chained format 2");
    auto deletion = words({1, 0, 1, 0});
    link(deletion, 2, coverage({1}));
    link(deletion, 6, words({0}));
    input = glyphs({1, 1, 2});
    substitute(layout({lookup(2, deletion)}), input);
    require(input.size() == 1 && input[0].glyph == 2, "GSUB deletion advances without skipping next input");
}
}

void check_substitution()
{
    basic_substitution();
    ligatures_and_context();
    reverse_and_filter();
    chained_formats_and_deletion();
}
}
