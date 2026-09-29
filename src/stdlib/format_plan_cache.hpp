#pragma once

#include "common/format_spec.hpp"

#include <memory>
#include <string>
#include <vector>

namespace tx_generated
{

inline constexpr std::size_t format_cache_entries = 32;
inline constexpr std::size_t format_cache_bytes = 256 * 1024;
inline constexpr std::size_t format_cache_template_bytes = 4096;

struct dynamic_format_part
{
    std::string literal;
    std::string name;
    tx::format_spec spec;
    char conversion = 0;
    bool field = false;
};

struct dynamic_format_plan
{
    std::string text;
    std::vector<dynamic_format_part> parts;
    std::size_t literal_bytes = 0;
};

std::shared_ptr<const dynamic_format_plan> find_format_plan(std::string_view text);
void remember_format_plan(dynamic_format_plan plan);

} // namespace tx_generated
