#include "stdlib/format_binding_cache.hpp"

#include <array>
#include <atomic>
#include <bit>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace tx_generated;

namespace
{

void require(bool value)
{
    if (!value)
    {
        throw std::runtime_error("format binding cache check failed");
    }
}

format_argument integer(std::int64_t value, const char* name = "")
{
    return {name, &tx_format_argument_i64, std::bit_cast<std::uint64_t>(value)};
}

std::string warm(const std::string& text, std::span<const format_argument> positional,
    std::span<const format_argument> keywords = {})
{
    const auto first = format_direct(text, positional, keywords);
    require(format_direct(text, positional, keywords) == first);
    return first;
}

void values_and_signatures()
{
    format_bindings.clear();
    std::array positional{integer(7)};
    std::array keywords{integer(11, "x"), integer(22, "y")};
    const std::string text = "{x}:{y}:{}:{x}";
    require(warm(text, positional, keywords) == "11:22:7:11");
    const auto syntax = find_format_plan(text);
    auto active = find_format_binding(syntax, {positional, keywords});
    require(active && format_bindings.size() == 1);
    positional[0].bits = 8;
    keywords[0].bits = 33;
    require(format_direct(text, positional, keywords) == "33:22:8:33");
    require(find_format_binding(syntax, {positional, keywords}) == active);
    std::swap(keywords[0], keywords[1]);
    require(format_direct(text, positional, keywords) == "33:22:8:33");
    require(format_bindings.size() == 2);
    // 没有关键字时，名称字段按原规则消耗位置参数；不能复用关键字绑定。
    std::array unnamed{integer(1), integer(2), integer(3)};
    require(format_direct(text, unnamed, {}) == "1:2:3:1");
    require(format_bindings.size() == 3);

    require(warm("typed:{:d}", positional) == "typed:8");
    const auto typed = find_format_plan("typed:{:d}");
    auto typed_binding = find_format_binding(typed, {positional, {}});
    positional[0] = {"", &tx_format_argument_bool, 1};
    bool failed = false;
    try
    {
        (void)format_direct("typed:{:d}", positional, {});
    }
    catch (const std::runtime_error&)
    {
        failed = true;
    }
    require(failed && find_format_binding(typed, {positional, {}}) == typed_binding);
    positional[0] = integer(9);
    require(format_direct("typed:{:d}", positional, {}) == "typed:9");
}

void budgets()
{
    format_bindings.clear();
    std::vector<format_argument> positional(format_binding_arguments, integer(1));
    require(warm("budget:{0}", positional) == "budget:1");
    const auto syntax = find_format_plan("budget:{0}");
    require(find_format_binding(syntax, {positional, {}}) != nullptr);
    const auto count = format_bindings.size();
    positional.push_back(integer(2));
    require(warm("budget:{0}", positional) == "budget:1");
    require(!find_format_binding(syntax, {positional, {}}));
    positional.pop_back();
    std::array keywords{integer(3, "x")};
    require(warm("budget:{0}", positional, keywords) == "budget:1");
    require(!find_format_binding(syntax, {positional, keywords}));
    require(format_bindings.size() == count);

    std::string dense;
    for (std::size_t index = 0; index < format_binding_parts + 1; ++index)
    {
        dense += "{0}";
    }
    require(warm(dense, positional) == std::string(format_binding_parts + 1, '1'));
    require(!find_format_binding(find_format_plan(dense), {positional, {}}));
    std::string long_name(format_cache_template_bytes + 1, 'x');
    keywords[0].name = long_name.c_str();
    require(warm("budget:{0}", std::span(positional).first(1), keywords) == "budget:1");
    require(!find_format_binding(syntax, {std::span(positional).first(1), keywords}));
    require(format_bindings.size() == count);
}

void populate(std::int64_t value)
{
    std::array positional{integer(value)};
    for (std::size_t index = 0; index <= format_binding_entries; ++index)
    {
        const auto prefix = "evict" + std::to_string(index) + ":";
        require(warm(prefix + "{}", positional) == prefix + std::to_string(value));
    }
}

void nested_append(std::string& output, std::uint64_t bits, const tx::format_spec& spec,
    char conversion)
{
    populate(99);
    tx_format_argument_i64(output, bits, spec, conversion);
}

void eviction_and_threads()
{
    format_bindings.clear();
    std::array positional{integer(5)};
    require(warm("active:{}:{}", std::array{integer(5), integer(6)}) == "active:5:6");
    const auto syntax = find_format_plan("active:{}:{}");
    std::array nested{integer(5), integer(6)};
    auto active = find_format_binding(syntax, {nested, {}});
    nested[0].append = &nested_append;
    require(format_direct("active:{}:{}", nested, {}) == "active:5:6");
    require(!find_format_binding(syntax, {nested, {}}));
    require(active && active->slots.size() == 2 && active->syntax == syntax);
    require(format_bindings.size() == format_binding_entries);
    auto main_binding = format_bindings.front();
    std::atomic<bool> valid = true;
    std::jthread worker([&]
    {
        try
        {
            require(format_bindings.empty());
            values_and_signatures();
            budgets();
            populate(42);
            require(format_bindings.size() == format_binding_entries);
        }
        catch (...)
        {
            valid = false;
        }
    });
    worker.join();
    require(valid && format_bindings.front() == main_binding);
    require(format_direct("fresh:{}", positional, {}) == "fresh:5");
}

} // namespace

int main()
{
    values_and_signatures();
    budgets();
    eviction_and_threads();
    std::cout << "FORMAT_BINDING_CACHE_OK\n";
}
