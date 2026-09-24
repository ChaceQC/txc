#include "backend/cpp/external_direct_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <any>
#include <string>

namespace
{

const std::string& text_value(const void* value)
{
    return *static_cast<const std::string*>(value);
}

const tx_generated::tx_array& array_value(const void* value)
{
    return std::any_cast<const tx_generated::tx_array&>(
        *static_cast<const std::any*>(value));
}

} // namespace

using tx_generated::detail::invoke_checked;

extern "C" int txrt_string_contains(const void* text, const void* part,
                                      bool* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_fn_contains(text_value(text), text_value(part));
    });
}

extern "C" int txrt_string_starts_with(const void* text, const void* prefix,
                                        bool* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_fn_starts_with(text_value(text),
                                                  text_value(prefix));
    });
}

extern "C" int txrt_string_ends_with(const void* text, const void* suffix,
                                      bool* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_fn_ends_with(text_value(text),
                                                text_value(suffix));
    });
}

extern "C" int txrt_string_find(const void* text, const void* part,
                                  std::int64_t* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_fn_find(text_value(text), text_value(part));
    });
}

extern "C" int txrt_string_slice(const void* text, std::int64_t start,
                                   std::int64_t end, void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::string(tx_generated::tx_fn_slice(text_value(text),
                                                              start, end));
    });
}

extern "C" int txrt_string_replace(const void* text, const void* old,
                                     const void* replacement,
                                     void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::string(tx_generated::tx_fn_replace(
            text_value(text), text_value(old), text_value(replacement)));
    });
}

extern "C" int txrt_string_split(const void* text, const void* separator,
                                   void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::any(tx_generated::tx_fn_split(
            text_value(text), text_value(separator)));
    });
}

extern "C" int txrt_string_join(const void* parts, const void* separator,
                                  void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::string(tx_generated::tx_fn_join(
            array_value(parts), text_value(separator)));
    });
}

extern "C" int txrt_string_trim(const void* text, void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::string(tx_generated::tx_fn_trim(text_value(text)));
    });
}

extern "C" int txrt_string_lower(const void* text, void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::string(tx_generated::tx_fn_lower(text_value(text)));
    });
}

extern "C" int txrt_string_upper(const void* text, void** result) noexcept
{
    return invoke_checked([&] {
        *result = new std::string(tx_generated::tx_fn_upper(text_value(text)));
    });
}
