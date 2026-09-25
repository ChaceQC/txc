#include "backend/cpp/parse_abi.hpp"
#include "backend/cpp/error_result.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/parse.hpp"

#include <any>
#include <string>

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;
using tx_generated::detail::make_result_value;

extern "C" int txrt_parse_try_parse_int(const void* text, std::int64_t base,
    const char* result_type, const char* error_type, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(make_result_value(result_type, error_type,
            tx_generated::try_parse_int(*static_cast<const std::string*>(text), base)));
    });
}

extern "C" int txrt_parse_try_parse_float(const void* text,
    const char* result_type, const char* error_type, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(make_result_value(result_type, error_type,
            tx_generated::try_parse_float(*static_cast<const std::string*>(text))));
    });
}

extern "C" int txrt_parse_parse_int(const void* text, std::int64_t base,
                                   std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        auto parsed = tx_generated::try_parse_int(*static_cast<const std::string*>(text), base);
        if (!parsed.ok)
        {
            throw tx_generated::runtime_failure(std::move(parsed.error));
        }
        *result = parsed.value;
    });
}

extern "C" int txrt_parse_parse_float(const void* text, double* result) noexcept
{
    return invoke_checked([&]
    {
        auto parsed = tx_generated::try_parse_float(*static_cast<const std::string*>(text));
        if (!parsed.ok)
        {
            throw tx_generated::runtime_failure(std::move(parsed.error));
        }
        *result = parsed.value;
    });
}
