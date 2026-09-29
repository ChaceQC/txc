#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace performance_equivalence
{

struct parse_error
{
    std::string kind;
    std::string code;
    std::string message;
};

struct int_result
{
    bool ok = false;
    std::int64_t value = 0;
    parse_error error;
};

[[nodiscard]] int_result try_parse_int(std::string_view text, std::int64_t base);
[[nodiscard]] bool parse_int_core(std::string_view text);

}
