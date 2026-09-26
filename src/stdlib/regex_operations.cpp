#include "stdlib/regex_internal.hpp"

#include "stdlib/error.hpp"

namespace tx_generated
{
namespace
{

constexpr std::size_t output_limit = 32 * 1024 * 1024;

void append_capped(std::string& output, std::string_view part)
{
    if (part.size() > output_limit - output.size())
    {
        throw runtime_failure({tx::error_kind::runtime, "size_limit",
                               "正则替换输出超过 32 MiB"});
    }
    output.append(part);
}

[[noreturn]] void invalid_replacement(std::size_t offset)
{
    throw runtime_failure({tx::error_kind::parse, "invalid_replacement",
        "正则替换模板第 " + std::to_string(offset) + " 个 UTF-8 字节无效"});
}

void append_group(std::string* output, const regex_match_value& match,
                  std::size_t group, std::size_t offset)
{
    if (group >= match.groups.size())
    {
        invalid_replacement(offset);
    }
    if (output)
    {
        append_capped(*output, match.groups[group]);
    }
}

void expand_template(std::string* output, std::string_view replacement,
                     const regex_state& state, const regex_match_value& match)
{
    for (std::size_t index = 0; index < replacement.size(); ++index)
    {
        if (replacement[index] != '$')
        {
            if (output)
            {
                append_capped(*output, replacement.substr(index, 1));
            }
            continue;
        }
        const auto dollar = index;
        if (++index == replacement.size())
        {
            invalid_replacement(dollar);
        }
        if (replacement[index] == '$')
        {
            if (output)
            {
                append_capped(*output, "$");
            }
            continue;
        }
        if (replacement[index] >= '0' && replacement[index] <= '9')
        {
            std::size_t group = replacement[index] - '0';
            if (index + 1 < replacement.size() &&
                replacement[index + 1] >= '0' && replacement[index + 1] <= '9')
            {
                group = group * 10 + replacement[++index] - '0';
            }
            append_group(output, match, group, dollar);
            continue;
        }
        if (replacement[index] != '{')
        {
            invalid_replacement(dollar);
        }
        const auto end = replacement.find('}', index + 1);
        if (end == std::string_view::npos || end == index + 1)
        {
            invalid_replacement(dollar);
        }
        const std::string name(replacement.substr(index + 1, end - index - 1));
        const auto found = state.named_groups.find(name);
        if (found == state.named_groups.end())
        {
            invalid_replacement(dollar);
        }
        append_group(output, match, found->second, dollar);
        index = end;
    }
}

} // namespace

std::vector<std::string> regex_find_all(const regex_pattern& pattern,
                                        std::string_view text)
{
    std::vector<std::string> result;
    regex_detail::scan_matches(*pattern.state, text,
        [&](const regex_match_value& found)
        {
            result.push_back(found.text);
        }, false);
    return result;
}

std::string regex_replace(const regex_pattern& pattern,
    std::string_view text, std::string_view replacement)
{
    regex_match_value shape;
    shape.groups.resize(pattern.state->group_names.size());
    expand_template(nullptr, replacement, *pattern.state, shape);
    std::string result;
    std::size_t consumed = 0;
    regex_detail::scan_matches(*pattern.state, text,
        [&](const regex_match_value& found)
        {
            const auto start = static_cast<std::size_t>(found.start_byte);
            const auto end = static_cast<std::size_t>(found.end_byte);
            append_capped(result, text.substr(consumed, start - consumed));
            expand_template(&result, replacement, *pattern.state, found);
            consumed = end;
        });
    append_capped(result, text.substr(consumed));
    return result;
}

std::vector<std::string> regex_split(const regex_pattern& pattern,
                                     std::string_view text)
{
    std::vector<std::string> result;
    std::size_t consumed = 0;
    regex_detail::scan_matches(*pattern.state, text,
        [&](const regex_match_value& found)
        {
            if (result.size() >= 99'999)
            {
                throw runtime_failure({tx::error_kind::runtime, "regex_limit",
                                       "正则拆分片段超过 100000 个"});
            }
            const auto start = static_cast<std::size_t>(found.start_byte);
            result.emplace_back(text.substr(consumed, start - consumed));
            consumed = static_cast<std::size_t>(found.end_byte);
        }, false);
    result.emplace_back(text.substr(consumed));
    return result;
}

} // namespace tx_generated
