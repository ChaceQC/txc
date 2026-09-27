#include "backend/cpp/format_static_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/format_internal.hpp"

namespace
{

template<class value_type>
int append_value(void* output, const value_type& value,
    const tx::format_spec* spec, char conversion) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        *static_cast<std::string*>(output) +=
            tx_generated::format_field_value(value, *spec, conversion);
    });
}

} // namespace

extern "C" int txrt_format_begin(void** result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        *result = tx_generated::detail::make_handle<std::string>();
    });
}

extern "C" int txrt_format_literal(void* result, const char* text,
    std::uint64_t length) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        static_cast<std::string*>(result)->append(text, length);
    });
}

extern "C" int txrt_format_append_i64(void* result, std::int64_t value,
    const tx::format_spec* spec, char conversion) noexcept
{
    return append_value(result, value, spec, conversion);
}

extern "C" int txrt_format_append_f64(void* result, double value,
    const tx::format_spec* spec, char conversion) noexcept
{
    return append_value(result, value, spec, conversion);
}

extern "C" int txrt_format_append_bool(void* result, bool value,
    const tx::format_spec* spec, char conversion) noexcept
{
    return append_value(result, value, spec, conversion);
}

extern "C" int txrt_format_append_str(void* result, const void* value,
    const tx::format_spec* spec, char conversion) noexcept
{
    return append_value(result, *static_cast<const std::string*>(value), spec, conversion);
}
