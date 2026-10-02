#include "stdlib/native_gui/core/line_break.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace tx::ui;

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    std::ifstream input(argv[1]);
    if (!input)
    {
        throw std::runtime_error("无法打开 Unicode 16.0 官方断行用例");
    }
    std::string line;
    unsigned line_number = 0, cases = 0, failures = 0;
    while (std::getline(input, line))
    {
        ++line_number;
        std::istringstream tokens(line.substr(0, line.find('#')));
        std::string token;
        std::u32string text;
        std::vector<bool> expected;
        while (tokens >> token)
        {
            if (token == "÷" || token == "×")
            {
                expected.push_back(token == "÷");
            }
            else
            {
                text.push_back(static_cast<char32_t>(std::stoul(token, nullptr, 16)));
            }
        }
        if (expected.empty())
        {
            continue;
        }
        ++cases;
        const auto actual = line_breaks(text);
        bool matches = actual.size() == expected.size();
        for (std::size_t index = 0; matches && index < actual.size(); ++index)
        {
            matches = (actual[index] != line_break_kind::prohibited) == expected[index];
        }
        if (!matches)
        {
            if (++failures <= 8)
            {
                std::cerr << "line " << line_number << ": " << line << '\n';
                std::cerr << "actual allowed:";
                for (std::size_t index = 0; index < actual.size(); ++index)
                {
                    if (actual[index] != line_break_kind::prohibited)
                    {
                        std::cerr << ' ' << index;
                    }
                }
                std::cerr << '\n';
            }
        }
    }
    std::cout << "Unicode 16.0 line break cases=" << cases << " failures=" << failures << '\n';
    if (!cases || failures)
    {
        return 1;
    }
    const auto controls = line_breaks(U"a\r\nb\v\f\x85\u2028\u2029z");
    for (const auto index : {3, 5, 6, 7, 8, 9, 10})
    {
        if (controls.at(index) != line_break_kind::mandatory)
        {
            throw std::runtime_error("mandatory line break mismatch");
        }
    }
    if (line_breaks(U"").front() != line_break_kind::mandatory)
    {
        throw std::runtime_error("empty line break mismatch");
    }
}
