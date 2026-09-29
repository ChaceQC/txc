#include "backend/cpp/parse_abi.hpp"
#include "backend/cpp/error_result.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/parse.hpp"

#include <any>
#include <string>

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;
using tx_generated::detail::make_result_value;

extern "C" int txrt_parse_int_scalar_context(void* context, const void* text, std::int64_t base,
    bool* ok, std::int64_t* value, std::int64_t* error) noexcept
{
    return invoke_checked([&]
    {
        const auto parsed = tx_generated::parse_int_scalar(
            tx_generated::detail::text_value(text), base);
        *ok = parsed.error == tx_generated::parse_error::none;
        *value = parsed.value;
        *error = static_cast<std::int64_t>(parsed.error);
    }, tx::error_kind::runtime,
        static_cast<tx_generated::detail::runtime_context*>(context));
}

extern "C" int txrt_parse_float_scalar_context(void* context, const void* text,
    bool* ok, double* value, std::int64_t* error) noexcept
{
    return invoke_checked([&]
    {
        const auto parsed = tx_generated::parse_float_scalar(
            tx_generated::detail::text_value(text));
        *ok = parsed.error == tx_generated::parse_error::none;
        *value = parsed.value;
        *error = static_cast<std::int64_t>(parsed.error);
    }, tx::error_kind::runtime,
        static_cast<tx_generated::detail::runtime_context*>(context));
}

extern "C" int txrt_parse_int_scalar(const void* text, std::int64_t base,
    bool* ok, std::int64_t* value, std::int64_t* error) noexcept
{
    return txrt_parse_int_scalar_context(nullptr, text, base, ok, value, error);
}

extern "C" int txrt_parse_float_scalar(const void* text,
    bool* ok, double* value, std::int64_t* error) noexcept
{
    return txrt_parse_float_scalar_context(nullptr, text, ok, value, error);
}

namespace
{

template<class value_type>
int materialize(bool ok, value_type value, std::int64_t error,
    const char* result_type, const char* error_type, void** result) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::operation_result<value_type> parsed{ok, value,
            tx_generated::materialize_parse_error(static_cast<tx_generated::parse_error>(error))};
        *result = make_handle<std::any>(make_result_value(result_type, error_type,
                                                         std::move(parsed)));
    });
}

} // namespace

extern "C" int txrt_parse_materialize_int(bool ok, std::int64_t value,
    std::int64_t error, const char* result_type, const char* error_type,
    void** result) noexcept
{
    return materialize(ok, value, error, result_type, error_type, result);
}

extern "C" int txrt_parse_materialize_float(bool ok, double value,
    std::int64_t error, const char* result_type, const char* error_type,
    void** result) noexcept
{
    return materialize(ok, value, error, result_type, error_type, result);
}

extern "C" int txrt_parse_try_parse_int(const void* text, std::int64_t base,
    const char* result_type, const char* error_type, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(make_result_value(result_type, error_type,
            tx_generated::try_parse_int(tx_generated::detail::text_value(text), base)));
    });
}

extern "C" int txrt_parse_try_parse_float(const void* text,
    const char* result_type, const char* error_type, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(make_result_value(result_type, error_type,
            tx_generated::try_parse_float(tx_generated::detail::text_value(text))));
    });
}

extern "C" int txrt_parse_parse_int(const void* text, std::int64_t base,
                                   std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        auto parsed = tx_generated::try_parse_int(tx_generated::detail::text_value(text), base);
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
        auto parsed = tx_generated::try_parse_float(tx_generated::detail::text_value(text));
        if (!parsed.ok)
        {
            throw tx_generated::runtime_failure(std::move(parsed.error));
        }
        *result = parsed.value;
    });
}
