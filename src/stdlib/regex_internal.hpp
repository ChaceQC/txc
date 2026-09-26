#pragma once

#include "stdlib/regex.hpp"

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace tx_generated
{

struct regex_code_deleter
{
    void operator()(pcre2_code* value) const noexcept;
};

struct regex_state
{
    std::unique_ptr<pcre2_code, regex_code_deleter> code;
    std::size_t max_input_bytes = 0;
    std::uint32_t match_limit = 0;
    std::uint32_t depth_limit = 0;
    std::shared_ptr<cancellation_state> cancel;
    std::vector<std::string> group_names;
    std::unordered_map<std::string, std::size_t> named_groups;
};

namespace regex_detail
{

void check_input(const regex_state& state, std::string_view text);
void check_cancel(const regex_state& state);
regex_match_value execute_match(const regex_state& state,
    std::string_view text, std::size_t offset, std::uint32_t options,
    bool include_scalar_offsets = true, bool include_groups = true);
std::size_t next_scalar(std::string_view text, std::size_t offset);
void scan_matches(const regex_state& state, std::string_view text,
    const std::function<void(const regex_match_value&)>& visit,
    bool include_groups = true);

} // namespace regex_detail

} // namespace tx_generated
