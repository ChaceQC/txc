#include "backend/cpp/format_static_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/format_internal.hpp"
#include <bit>

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

extern "C" void tx_format_fast_i64(std::string& output, std::uint64_t bits,
    const tx::format_spec&, char)
{
    tx_generated::append_format_integer(output, std::bit_cast<std::int64_t>(bits));
}

extern "C" void tx_format_fast_bool(std::string& output, std::uint64_t bits,
    const tx::format_spec&, char)
{
    tx_generated::append_format_bool(output, bits != 0);
}

extern "C" void tx_format_fast_str(std::string& output, std::uint64_t bits,
    const tx::format_spec&, char)
{
    tx_generated::append_format_text(output,
        tx_generated::detail::text_value(reinterpret_cast<const void*>(bits)));
}

extern "C" void tx_format_fast_literal(std::string& output, std::uint64_t bits,
    const tx::format_spec& spec, char)
{
    tx_generated::append_format_text(output, std::string_view(
        reinterpret_cast<const char*>(bits), static_cast<std::size_t>(spec.precision)));
}

extern "C" int txrt_format_execute(const tx_generated::static_format_step* steps,
    std::uint64_t count, const std::uint64_t* arguments, const char* prefix,
    std::uint64_t length, std::uint64_t capacity, void** result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        // 整次格式化由一个 RAII 字符串持有，失败不发布半成品，也不登记中间根。
        std::string output;
        if (capacity > output.max_size())
        {
            throw std::length_error("format 输出长度过大");
        }
        output.reserve(capacity);
        output.append(prefix, length);
        for (std::uint64_t index = 0; index < count; ++index)
        {
            const auto& step = steps[index];
            step.append(output, arguments[step.argument], step.spec, static_cast<char>(step.conversion));
            output.append(step.tail, step.length);
        }
        *result = tx_generated::detail::make_handle<std::string>(std::move(output));
    });
}

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

extern "C" void tx_format_tail(std::string& output, const char* text, std::uint64_t length)
{
    output.append(text, length);
}

extern "C" int txrt_format_specialized(
    void (*execute)(std::string&, const std::uint64_t*), const std::uint64_t* arguments,
    const char* prefix, std::uint64_t length, std::uint64_t capacity, void** result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        // 生成函数不持有资源；任何 append 异常都回到此处销毁尚未发布的输出。
        std::string output;
        if (capacity > output.max_size())
        {
            throw std::length_error("format 输出长度过大");
        }
        output.reserve(capacity);
        output.append(prefix, length);
        execute(output, arguments);
        *result = tx_generated::detail::make_handle<std::string>(std::move(output));
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
