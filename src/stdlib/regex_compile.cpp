#include "stdlib/regex_internal.hpp"

#include "stdlib/error.hpp"

#include <chrono>
#include <mutex>
#include <stdexcept>

namespace tx_generated
{

void regex_code_deleter::operator()(pcre2_code* value) const noexcept
{
    if (value)
    {
        pcre2_code_free(value);
    }
}

namespace
{

std::uint32_t compile_flags(std::string_view flags)
{
    std::uint32_t result = PCRE2_UTF | PCRE2_UCP;
    bool seen[256]{};
    for (const unsigned char flag : flags)
    {
        if (seen[flag])
        {
            throw runtime_failure({tx::error_kind::parse, "invalid_flags",
                                   "正则标志不能重复"});
        }
        seen[flag] = true;
        switch (flag)
        {
        case 'i':
            result |= PCRE2_CASELESS;
            break;
        case 'm':
            result |= PCRE2_MULTILINE;
            break;
        case 's':
            result |= PCRE2_DOTALL;
            break;
        case 'x':
            result |= PCRE2_EXTENDED;
            break;
        default:
            throw runtime_failure({tx::error_kind::parse, "invalid_flags",
                                   "正则标志只能是 i、m、s、x"});
        }
    }
    return result;
}

void check_limits(std::string_view pattern, std::int64_t max_input,
                  std::int64_t match_limit, std::int64_t depth_limit)
{
    if (pattern.size() > 1024 * 1024 || max_input < 1 ||
        max_input > 32 * 1024 * 1024 || match_limit < 1 ||
        match_limit > 10'000'000 || depth_limit < 1 || depth_limit > 100'000)
    {
        throw runtime_failure({tx::error_kind::runtime, "size_limit",
                               "正则模式长度或执行限额超出允许范围"});
    }
}

void load_group_names(regex_state& state)
{
    std::uint32_t count = 0;
    std::uint32_t named_count = 0;
    std::uint32_t entry_size = 0;
    PCRE2_SPTR table = nullptr;
    pcre2_pattern_info(state.code.get(), PCRE2_INFO_CAPTURECOUNT, &count);
    pcre2_pattern_info(state.code.get(), PCRE2_INFO_NAMECOUNT, &named_count);
    if (count > 256)
    {
        throw runtime_failure({tx::error_kind::runtime, "regex_limit",
                               "正则捕获组超过 256 个"});
    }
    state.group_names.resize(static_cast<std::size_t>(count) + 1);
    if (named_count == 0)
    {
        return;
    }
    pcre2_pattern_info(state.code.get(), PCRE2_INFO_NAMEENTRYSIZE, &entry_size);
    pcre2_pattern_info(state.code.get(), PCRE2_INFO_NAMETABLE, &table);
    for (std::uint32_t index = 0; index < named_count; ++index)
    {
        const auto* entry = table + index * entry_size;
        const auto group = static_cast<std::size_t>((entry[0] << 8) | entry[1]);
        const std::string name(reinterpret_cast<const char*>(entry + 2));
        if (group < state.group_names.size())
        {
            state.group_names[group] = name;
            state.named_groups.emplace(name, group);
        }
    }
}

} // namespace

namespace regex_detail
{

void check_cancel(const regex_state& state)
{
    if (!state.cancel)
    {
        return;
    }
    std::lock_guard lock(state.cancel->mutex);
    if (state.cancel->cancelled ||
        (state.cancel->deadline &&
         std::chrono::steady_clock::now() >= *state.cancel->deadline))
    {
        throw runtime_failure({tx::error_kind::cancelled, "cancelled",
                               "正则执行已取消或超过截止时间"});
    }
}

} // namespace regex_detail

regex_pattern regex_compile(std::string_view pattern, std::string_view flags,
    std::int64_t max_input_bytes, std::int64_t match_limit,
    std::int64_t depth_limit, std::shared_ptr<cancellation_state> cancel)
{
    check_limits(pattern, max_input_bytes, match_limit, depth_limit);
    auto state = std::make_shared<regex_state>();
    state->max_input_bytes = static_cast<std::size_t>(max_input_bytes);
    state->match_limit = static_cast<std::uint32_t>(match_limit);
    state->depth_limit = static_cast<std::uint32_t>(depth_limit);
    state->cancel = std::move(cancel);
    regex_detail::check_cancel(*state);
    auto options = compile_flags(flags);
    if (state->cancel)
    {
        options |= PCRE2_AUTO_CALLOUT;
    }
    int error_code = 0;
    PCRE2_SIZE error_offset = 0;
    state->code.reset(pcre2_compile(
        reinterpret_cast<PCRE2_SPTR>(pattern.data()), pattern.size(),
        options, &error_code, &error_offset, nullptr));
    if (!state->code)
    {
        PCRE2_UCHAR message[256]{};
        pcre2_get_error_message(error_code, message, sizeof(message));
        throw runtime_failure({tx::error_kind::parse, "invalid_regex",
            "正则模式第 " + std::to_string(error_offset) + " 个 UTF-8 字节附近有语法错误：" +
            reinterpret_cast<const char*>(message)});
    }
    load_group_names(*state);
    return {std::move(state)};
}

} // namespace tx_generated
