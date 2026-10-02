#include "stdlib/native_gui/core/bidi.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <map>

using namespace tx::ui;

namespace
{
std::vector<int> numbers(const std::string& value)
{
    std::istringstream input(value);
    std::string token;
    std::vector<int> result;
    while (input >> token)
    {
        result.push_back(token == "x" ? -1 : std::stoi(token));
    }
    return result;
}

void check(const bidi_paragraph& result, const std::vector<int>& levels, const std::vector<int>& order,
    const std::string& location)
{
    const auto actual = result.line_levels(0, result.levels.size());
    if (actual.size() != levels.size())
    {
        throw std::runtime_error(location + " level count");
    }
    for (std::size_t i = 0; i < actual.size(); ++i)
    {
        if (levels[i] >= 0 && actual[i] != unsigned(levels[i]))
        {
            throw std::runtime_error(location + " level at " + std::to_string(i) + " got " +
                std::to_string(actual[i]) + " expected " + std::to_string(levels[i]));
        }
    }
    const auto visual = result.visual_order(0, result.levels.size());
    if (visual.size() != order.size())
    {
        throw std::runtime_error(location + " visual size");
    }
    for (std::size_t i = 0; i < visual.size(); ++i)
    {
        if (visual[i] != unsigned(order[i]))
        {
            throw std::runtime_error(location + " visual order");
        }
    }
}

std::size_t type_tests(const char* path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("cannot read BidiTest");
    }
    const std::map<std::string, bidi_type> names{
        {"L", bidi_type::l}, {"R", bidi_type::r}, {"AL", bidi_type::al}, {"EN", bidi_type::en},
        {"ES", bidi_type::es}, {"ET", bidi_type::et}, {"AN", bidi_type::an}, {"CS", bidi_type::cs},
        {"NSM", bidi_type::nsm}, {"BN", bidi_type::bn}, {"B", bidi_type::b}, {"S", bidi_type::s},
        {"WS", bidi_type::ws}, {"ON", bidi_type::on}, {"LRE", bidi_type::lre}, {"LRO", bidi_type::lro},
        {"RLE", bidi_type::rle}, {"RLO", bidi_type::rlo}, {"PDF", bidi_type::pdf}, {"LRI", bidi_type::lri},
        {"RLI", bidi_type::rli}, {"FSI", bidi_type::fsi}, {"PDI", bidi_type::pdi}};
    std::vector<int> levels, order;
    std::string line;
    std::size_t number = 0, count = 0;
    while (std::getline(input, line))
    {
        ++number;
        line = line.substr(0, line.find('#'));
        if (line.empty())
        {
            continue;
        }
        if (line.starts_with("@Levels:"))
        {
            levels = numbers(line.substr(8));
        }
        else if (line.starts_with("@Reorder:"))
        {
            order = numbers(line.substr(9));
        }
        else if (const auto split = line.find(';'); split != std::string::npos)
        {
            std::istringstream tokens(line.substr(0, split));
            std::string token;
            std::vector<bidi_type> types;
            while (tokens >> token)
            {
                types.push_back(names.at(token));
            }
            const auto mask = std::stoi(line.substr(split + 1), nullptr, 16);
            for (int direction = -1; direction <= 1; ++direction)
            {
                if (mask & (1 << (direction + 1)))
                {
                    check(resolve_bidi_types(types, direction), levels, order,
                        "BidiTest:" + std::to_string(number) + " direction " + std::to_string(direction));
                    ++count;
                }
            }
        }
    }
    return count;
}

std::size_t character_tests(const char* path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("cannot read BidiCharacterTest");
    }
    std::string line;
    std::size_t number = 0, count = 0;
    while (std::getline(input, line))
    {
        ++number;
        line = line.substr(0, line.find('#'));
        if (line.find(';') == std::string::npos)
        {
            continue;
        }
        std::istringstream record(line);
        std::string fields[5];
        for (auto& field : fields)
        {
            std::getline(record, field, ';');
        }
        std::istringstream scalars(fields[0]);
        std::string token;
        std::u32string text;
        while (scalars >> token)
        {
            text.push_back(char32_t(std::stoul(token, nullptr, 16)));
        }
        const auto direction = std::stoi(fields[1]);
        const auto result = resolve_bidi(text, direction == 2 ? -1 : direction);
        const auto location = "BidiCharacterTest:" + std::to_string(number);
        if (result.base != unsigned(std::stoi(fields[2])))
        {
            throw std::runtime_error(location + " paragraph direction");
        }
        check(result, numbers(fields[3]), numbers(fields[4]), location);
        ++count;
    }
    return count;
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
        const auto types = type_tests(argv[1]);
        const auto characters = character_tests(argv[2]);
        if (types != 770241 || characters != 91707)
        {
            throw std::runtime_error("Unicode 16.0 bidi test corpus is incomplete");
        }
        std::cout << "Unicode 16.0 UAX9 PASS: " << types << " type cases; " << characters << " character cases\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
