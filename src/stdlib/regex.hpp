#pragma once

#include "stdlib/cancellation.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tx_generated
{

struct regex_state;

struct regex_pattern
{
    std::shared_ptr<regex_state> state;
};

struct regex_match_value
{
    bool found = false;
    std::string text;
    std::int64_t start_byte = -1;
    std::int64_t end_byte = -1;
    std::int64_t start_scalar = -1;
    std::int64_t end_scalar = -1;
    std::vector<std::string> groups;
    std::vector<std::string> group_names;
    std::vector<std::int64_t> group_start_bytes;
    std::vector<std::int64_t> group_end_bytes;
};

regex_pattern regex_compile(std::string_view pattern, std::string_view flags,
    std::int64_t max_input_bytes, std::int64_t match_limit,
    std::int64_t depth_limit, std::shared_ptr<cancellation_state> cancel = {});
regex_match_value regex_search(const regex_pattern& pattern,
    std::string_view text, std::int64_t start_byte);
regex_match_value regex_match(const regex_pattern& pattern,
    std::string_view text, bool full);
std::vector<std::string> regex_find_all(const regex_pattern& pattern,
    std::string_view text);
std::string regex_replace(const regex_pattern& pattern,
    std::string_view text, std::string_view replacement);
std::vector<std::string> regex_split(const regex_pattern& pattern,
    std::string_view text);

} // namespace tx_generated
