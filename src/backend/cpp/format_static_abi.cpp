#include "backend/cpp/format_static_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/format_internal.hpp"

namespace
{

template<class action_type>
int append_part(void* output, const char* tail, std::uint64_t length,
    action_type action) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        auto& builder = tx_generated::detail::text_builder(output);
        action(builder);
        builder.append(tail, length);
    });
}

template<class value_type>
int append_value(void* output, const value_type& value,
    const tx::format_spec* spec, char conversion, const char* tail, std::uint64_t length) noexcept
{
    return append_part(output, tail, length, [&](std::string& builder)
    {
        tx_generated::append_format_value(builder, value, *spec, conversion);
    });
}

} // namespace

extern "C" int txrt_format_begin(void** result, std::uint64_t capacity,
    const char* text, std::uint64_t length) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        // 先登记根，后续 reserve/append 失败仍由原错误清理流程释放。
        *result = tx_generated::detail::make_text_builder();
        auto& builder = tx_generated::detail::text_builder(*result);
        if (capacity > builder.max_size())
        {
            throw std::length_error("format 输出长度过大");
        }
        builder.reserve(capacity);
        builder.append(text, length);
    });
}

extern "C" int txrt_format_append_i64(void* result, std::int64_t value,
    const tx::format_spec* spec, char conversion, const char* tail, std::uint64_t length) noexcept
{
    return append_value(result, value, spec, conversion, tail, length);
}

extern "C" int txrt_format_append_f64(void* result, double value,
    const tx::format_spec* spec, char conversion, const char* tail, std::uint64_t length) noexcept
{
    return append_value(result, value, spec, conversion, tail, length);
}

extern "C" int txrt_format_append_bool(void* result, bool value,
    const tx::format_spec* spec, char conversion, const char* tail, std::uint64_t length) noexcept
{
    return append_value(result, value, spec, conversion, tail, length);
}

extern "C" int txrt_format_append_str(void* result, const void* value,
    const tx::format_spec* spec, char conversion, const char* tail, std::uint64_t length) noexcept
{
    return append_value(result, tx_generated::detail::text_value(value), spec, conversion, tail, length);
}

extern "C" int txrt_format_plain_i64(void* result, std::int64_t value,
    const char* tail, std::uint64_t length) noexcept
{
    return append_part(result, tail, length, [&](std::string& builder)
    {
        tx_generated::append_format_integer(builder, value);
    });
}

extern "C" int txrt_format_plain_bool(void* result, bool value,
    const char* tail, std::uint64_t length) noexcept
{
    return append_part(result, tail, length, [&](std::string& builder)
    {
        tx_generated::append_format_bool(builder, value);
    });
}

extern "C" int txrt_format_plain_str(void* result, const void* value,
    const char* tail, std::uint64_t length) noexcept
{
    return append_part(result, tail, length, [&](std::string& builder)
    {
        tx_generated::append_format_text(builder, tx_generated::detail::text_value(value));
    });
}

extern "C" int txrt_format_plain_bytes(void* result, const char* value, std::uint64_t size,
    const char* tail, std::uint64_t length) noexcept
{
    return append_part(result, tail, length, [&](std::string& builder)
    {
        tx_generated::append_format_text(builder, std::string_view(value, size));
    });
}

extern "C" void txrt_format_finish(void* result) noexcept
{
    tx_generated::detail::publish_text_builder(result);
}
