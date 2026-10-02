#include "stdlib/native_gui/core/unicode.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace tx::ui;

int main(int argc, char** argv)
{
    if (argc != 2 && argc != 3)
    {
        return 2;
    }
    const auto sample = decode_utf8("中文 é 👨‍👩‍👧‍👦 🇨🇳");
    assert(encode_utf8(sample.scalars) == "中文 é 👨‍👩‍👧‍👦 🇨🇳");
    assert(sample.byte_offsets.back() == encode_utf8(sample.scalars).size());
    for (const auto* malformed : {"\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe4\xb8"})
    {
        bool rejected = false;
        try
        {
            decode_utf8(malformed);
        }
        catch (const std::invalid_argument&)
        {
            rejected = true;
        }
        assert(rejected);
    }
    std::ifstream input(argv[1]);
    if (!input)
    {
        throw std::runtime_error("无法打开官方字素边界数据");
    }
    std::string line;
    unsigned line_number = 0, cases = 0;
    while (std::getline(input, line))
    {
        ++line_number;
        line = line.substr(0, line.find('#'));
        std::istringstream tokens(line);
        std::string token;
        std::u32string text;
        std::vector<std::size_t> expected;
        while (tokens >> token)
        {
            if (token == "÷")
            {
                expected.push_back(text.size());
            }
            else if (token != "×")
            {
                text.push_back(static_cast<char32_t>(std::stoul(token, nullptr, 16)));
            }
        }
        if (!expected.empty())
        {
            const auto actual = argc == 3 ? word_boundaries(text) : grapheme_boundaries(text);
            if (actual != expected)
            {
                std::cerr << "Unicode boundary mismatch, line " << line_number << '\n';
                return 1;
            }
            ++cases;
        }
    }
    std::cout << "Unicode 16.0 " << (argc == 3 ? "word" : "grapheme") << " cases=" << cases << " PASS\n";
}
