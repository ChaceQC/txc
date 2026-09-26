#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/unicode_text.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <cstdint>
#include <string>

namespace
{

const std::string& text_value(const void* value)
{
    return *static_cast<const std::string*>(value);
}

tx_generated::string_vector string_values(const std::vector<std::string>& source)
{
    tx_generated::string_vector result;
    auto& values = result.data().values;
    values.reserve(source.size());
    for (const auto& item : source)
    {
        auto* handle = tx_generated::detail::make_handle<std::string>(item);
        values.emplace_back(handle);
        tx_generated::detail::destroy_handle(handle);
    }
    result.data().refresh();
    return result;
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_unicode_version(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::unicode_text::data_version());
    });
}

extern "C" int txrt_unicode_icu_version(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::unicode_text::icu_version());
    });
}

extern "C" int txrt_unicode_normalize(const void* text, const void* form,
                                        void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::unicode_text::normalize(
            text_value(text), text_value(form)));
    });
}

extern "C" int txrt_unicode_case_fold(const void* text,
                                        void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::unicode_text::case_fold(
            text_value(text)));
    });
}

extern "C" int txrt_unicode_to_lower(const void* text, const void* locale,
                                       void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::unicode_text::change_case(
            text_value(text), text_value(locale), false));
    });
}

extern "C" int txrt_unicode_to_upper(const void* text, const void* locale,
                                       void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::unicode_text::change_case(
            text_value(text), text_value(locale), true));
    });
}

extern "C" int txrt_unicode_code_points(const void* text,
                                          void** result) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::int_vector values;
        values.data().values = tx_generated::unicode_text::code_points(
            text_value(text));
        values.data().refresh();
        *result = make_handle<std::any>(std::move(values));
    });
}

extern "C" int txrt_unicode_graphemes(const void* text,
                                       void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(string_values(
            tx_generated::unicode_text::graphemes(text_value(text))));
    });
}

extern "C" int txrt_unicode_grapheme_count(const void* text,
                                             std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::unicode_text::grapheme_count(text_value(text));
    });
}

extern "C" int txrt_unicode_category(std::int64_t point,
                                      void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::unicode_text::category(point));
    });
}

extern "C" int txrt_unicode_display_width(const void* text,
                                            std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::unicode_text::display_width(text_value(text));
    });
}

extern "C" int txrt_unicode_compare(const void* left, const void* right,
    const void* locale, const void* strength, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::unicode_text::compare(text_value(left),
            text_value(right), text_value(locale), text_value(strength));
    });
}
