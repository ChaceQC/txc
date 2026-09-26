#include "stdlib/regex_internal.hpp"

#include "stdlib/error.hpp"

#include <chrono>
#include <memory>
#include <mutex>

namespace tx_generated
{
namespace
{

using match_handle = std::unique_ptr<pcre2_match_data,
    decltype(&pcre2_match_data_free)>;
using context_handle = std::unique_ptr<pcre2_match_context,
    decltype(&pcre2_match_context_free)>;

int cancellation_callout(pcre2_callout_block*, void* context) noexcept
{
    const auto& state = *static_cast<const regex_state*>(context);
    try
    {
        std::lock_guard lock(state.cancel->mutex);
        return state.cancel->cancelled ||
            (state.cancel->deadline &&
             std::chrono::steady_clock::now() >= *state.cancel->deadline)
            ? PCRE2_ERROR_CALLOUT : 0;
    }
    catch (...)
    {
        return PCRE2_ERROR_CALLOUT;
    }
}

std::int64_t scalar_offset(std::string_view text, std::size_t byte_offset)
{
    std::int64_t count = 0;
    for (std::size_t index = 0; index < byte_offset; ++index)
    {
        if ((static_cast<unsigned char>(text[index]) & 0xc0) != 0x80)
        {
            ++count;
        }
    }
    return count;
}

regex_match_value capture_result(const regex_state& state,
    std::string_view text, pcre2_match_data* data,
    bool include_scalar_offsets, bool include_groups)
{
    regex_match_value result;
    result.found = true;
    const auto* offsets = pcre2_get_ovector_pointer(data);
    result.start_byte = static_cast<std::int64_t>(offsets[0]);
    result.end_byte = static_cast<std::int64_t>(offsets[1]);
    result.text = std::string(text.substr(offsets[0], offsets[1] - offsets[0]));
    if (include_groups)
    {
        result.group_names = state.group_names;
        for (std::size_t index = 0; index < state.group_names.size(); ++index)
        {
            const auto start = offsets[index * 2];
            const auto end = offsets[index * 2 + 1];
            if (start == PCRE2_UNSET)
            {
                result.groups.emplace_back();
                result.group_start_bytes.push_back(-1);
                result.group_end_bytes.push_back(-1);
                continue;
            }
            result.groups.emplace_back(text.substr(start, end - start));
            result.group_start_bytes.push_back(static_cast<std::int64_t>(start));
            result.group_end_bytes.push_back(static_cast<std::int64_t>(end));
        }
    }
    if (include_scalar_offsets)
    {
        result.start_scalar = scalar_offset(text,
            static_cast<std::size_t>(result.start_byte));
        result.end_scalar = scalar_offset(text,
            static_cast<std::size_t>(result.end_byte));
    }
    return result;
}

void match_error(const regex_state& state, int code)
{
    if (code == PCRE2_ERROR_MATCHLIMIT || code == PCRE2_ERROR_DEPTHLIMIT ||
        code == PCRE2_ERROR_HEAPLIMIT || code == PCRE2_ERROR_JIT_STACKLIMIT)
    {
        throw runtime_failure({tx::error_kind::runtime, "regex_limit",
                               "正则匹配超过步数、深度或内存限额"});
    }
    if (code == PCRE2_ERROR_CALLOUT)
    {
        regex_detail::check_cancel(state);
    }
    if (code <= PCRE2_ERROR_UTF8_ERR1 && code >= PCRE2_ERROR_UTF8_ERR21)
    {
        throw runtime_failure({tx::error_kind::parse, "invalid_unicode",
                               "正则输入包含无效 UTF-8 序列"});
    }
    throw runtime_failure({tx::error_kind::runtime, "regex_failed",
                           "PCRE2 执行失败，错误码 " + std::to_string(code)});
}

} // namespace

namespace regex_detail
{

void check_input(const regex_state& state, std::string_view text)
{
    if (text.size() > state.max_input_bytes)
    {
        throw runtime_failure({tx::error_kind::runtime, "size_limit",
                               "正则输入超过编译时设定的字节上限"});
    }
    check_cancel(state);
}

std::size_t next_scalar(std::string_view text, std::size_t offset)
{
    if (offset >= text.size())
    {
        return text.size();
    }
    const auto first = static_cast<unsigned char>(text[offset]);
    const auto size = first < 0x80 ? 1U : first < 0xe0 ? 2U :
        first < 0xf0 ? 3U : 4U;
    return offset + size;
}

regex_match_value execute_match(const regex_state& state,
    std::string_view text, std::size_t offset, std::uint32_t options,
    bool include_scalar_offsets, bool include_groups)
{
    check_input(state, text);
    if (offset > text.size() ||
        (offset < text.size() &&
         (static_cast<unsigned char>(text[offset]) & 0xc0) == 0x80))
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_argument",
                               "正则起点必须位于 UTF-8 标量边界"});
    }
    match_handle data(pcre2_match_data_create_from_pattern(state.code.get(),
        nullptr), &pcre2_match_data_free);
    context_handle context(pcre2_match_context_create(nullptr),
        &pcre2_match_context_free);
    if (!data || !context)
    {
        throw std::bad_alloc();
    }
    pcre2_set_match_limit(context.get(), state.match_limit);
    pcre2_set_depth_limit(context.get(), state.depth_limit);
    pcre2_set_heap_limit(context.get(), 32768);
    if (state.cancel)
    {
        pcre2_set_callout(context.get(), cancellation_callout,
            const_cast<regex_state*>(&state));
    }
    const auto code = pcre2_match(state.code.get(),
        reinterpret_cast<PCRE2_SPTR>(text.data()), text.size(), offset,
        options, data.get(), context.get());
    if (code == PCRE2_ERROR_NOMATCH)
    {
        return {};
    }
    if (code < 0)
    {
        match_error(state, code);
    }
    return capture_result(state, text, data.get(), include_scalar_offsets,
                          include_groups);
}

void scan_matches(const regex_state& state, std::string_view text,
    const std::function<void(const regex_match_value&)>& visit,
    bool include_groups)
{
    std::size_t offset = 0;
    bool retry_nonempty = false;
    std::size_t count = 0;
    while (offset <= text.size())
    {
        const auto options = retry_nonempty
            ? PCRE2_ANCHORED | PCRE2_NOTEMPTY_ATSTART : 0U;
        auto found = execute_match(state, text, offset, options, false,
                                   include_groups);
        if (!found.found)
        {
            if (!retry_nonempty || offset == text.size())
            {
                return;
            }
            offset = next_scalar(text, offset);
            retry_nonempty = false;
            continue;
        }
        if (++count > 100'000)
        {
            throw runtime_failure({tx::error_kind::runtime, "regex_limit",
                                   "正则匹配结果超过 100000 个"});
        }
        visit(found);
        offset = static_cast<std::size_t>(found.end_byte);
        retry_nonempty = found.start_byte == found.end_byte;
    }
}

} // namespace regex_detail

regex_match_value regex_search(const regex_pattern& pattern,
    std::string_view text, std::int64_t start_byte)
{
    if (start_byte < 0)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_argument",
                               "正则搜索起点不能为负"});
    }
    return regex_detail::execute_match(*pattern.state, text,
        static_cast<std::size_t>(start_byte), 0);
}

regex_match_value regex_match(const regex_pattern& pattern,
    std::string_view text, bool full)
{
    const auto options = full ? PCRE2_ANCHORED | PCRE2_ENDANCHORED :
        PCRE2_ANCHORED;
    return regex_detail::execute_match(*pattern.state, text, 0, options);
}

} // namespace tx_generated
