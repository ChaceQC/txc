#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/regex.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <cstdint>
#include <string>
#include <vector>

namespace
{

const std::string& text_value(const void* value)
{
    return tx_generated::detail::text_value(value);
}

const tx_generated::regex_pattern& pattern_value(const void* value)
{
    return std::any_cast<const tx_generated::regex_pattern&>(
        *static_cast<const std::any*>(value));
}

tx_generated::string_vector text_vector(std::vector<std::string> source)
{
    tx_generated::string_vector result;
    auto& values = result.data().values;
    values.reserve(source.size());
    for (auto& item : source)
    {
        auto* handle = tx_generated::detail::make_handle<std::string>(std::move(item));
        values.emplace_back(handle);
        tx_generated::detail::destroy_handle(handle);
    }
    result.data().refresh();
    return result;
}

tx_generated::int_vector number_vector(
    std::vector<std::int64_t> source)
{
    tx_generated::int_vector result;
    result.data().values = std::move(source);
    result.data().refresh();
    return result;
}

tx_generated::dynamic_struct match_struct(const char* type_name,
                                          tx_generated::regex_match_value value)
{
    tx_generated::struct_fields fields(10);
    fields[0] = {"found", value.found};
    fields[1] = {"text", std::move(value.text)};
    fields[2] = {"start_byte", value.start_byte};
    fields[3] = {"end_byte", value.end_byte};
    fields[4] = {"start_scalar", value.start_scalar};
    fields[5] = {"end_scalar", value.end_scalar};
    fields[6] = {"groups", text_vector(std::move(value.groups))};
    fields[7] = {"group_names", text_vector(std::move(value.group_names))};
    fields[8] = {"group_start_bytes", number_vector(std::move(value.group_start_bytes))};
    fields[9] = {"group_end_bytes", number_vector(std::move(value.group_end_bytes))};
    return tx_generated::dynamic_struct(tx_generated::dynamic_struct_data{
        type_name, "regex_match", std::move(fields)});
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_regex_projected(const void* pattern, const void* input,
    std::int64_t start, std::int64_t mode, bool* found, void** text, std::int64_t* offsets) noexcept
{
    return invoke_checked([&]
    {
        auto result = tx_generated::regex_projected_match(pattern_value(pattern), text_value(input), start, mode);
        *text = make_handle<std::string>(std::move(result.text));
        *found = result.found;
        offsets[0] = result.start_byte;
        offsets[1] = result.end_byte;
        offsets[2] = result.start_scalar;
        offsets[3] = result.end_scalar;
    });
}

extern "C" int txrt_regex_compile(const void* pattern, const void* flags,
    std::int64_t max_input, std::int64_t match_limit, std::int64_t depth_limit,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::regex_compile(
            text_value(pattern), text_value(flags), max_input, match_limit,
            depth_limit));
    });
}

extern "C" int txrt_regex_compile_with_cancel(const void* pattern,
    const void* flags, std::int64_t max_input, std::int64_t match_limit,
    std::int64_t depth_limit, const void* token, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& cancellation = std::any_cast<const tx_generated::cancel_token&>(
            *static_cast<const std::any*>(token));
        *result = make_handle<std::any>(tx_generated::regex_compile(
            text_value(pattern), text_value(flags), max_input, match_limit,
            depth_limit, cancellation.state));
    });
}

extern "C" int txrt_regex_search(const void* pattern, const void* text,
    std::int64_t start_byte, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(match_struct(type_name,
            tx_generated::regex_search(pattern_value(pattern), text_value(text),
                                       start_byte)));
    });
}

extern "C" int txrt_regex_match(const void* pattern, const void* text,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(match_struct(type_name,
            tx_generated::regex_match(pattern_value(pattern), text_value(text),
                                      false)));
    });
}

extern "C" int txrt_regex_full_match(const void* pattern, const void* text,
    const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(match_struct(type_name,
            tx_generated::regex_match(pattern_value(pattern), text_value(text),
                                      true)));
    });
}

extern "C" int txrt_regex_find_all(const void* pattern, const void* text,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(text_vector(tx_generated::regex_find_all(
            pattern_value(pattern), text_value(text))));
    });
}

extern "C" int txrt_regex_replace(const void* pattern, const void* text,
    const void* replacement, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::regex_replace(
            pattern_value(pattern), text_value(text), text_value(replacement)));
    });
}

extern "C" int txrt_regex_split(const void* pattern, const void* text,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(text_vector(tx_generated::regex_split(
            pattern_value(pattern), text_value(text))));
    });
}
