#include "backend/cpp/json_stream_abi.hpp"
#include "backend/cpp/format_abi.hpp"
#include "stdlib/json_stream.hpp"

namespace
{

tx_generated::json_limits read_limits(const void* value)
{
    using tx_generated::format_abi::member;
    return {member<std::int64_t>(value, 0), member<std::int64_t>(value, 1),
        member<std::int64_t>(value, 2)};
}

} // namespace

using namespace tx_generated;
using namespace tx_generated::format_abi;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_json_default_limits(const char* type, void** result) noexcept
{
    return invoke_checked([&]
    {
        const json_limits limits;
        *result = structure(type, "limits", {{"max_bytes", limits.max_bytes},
            {"max_value_bytes", limits.max_value_bytes}, {"max_depth", limits.max_depth}});
    });
}

extern "C" int txrt_json_next(const void* source, const char* type, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = option(type, json_next(argument<json_reader>(source)));
    });
}

extern "C" int txrt_json_write_value(const void* target, const void* data) noexcept
{
    return invoke_checked([&]
    {
        json_write_value(argument<json_writer>(target), value(data));
    });
}

extern "C" int txrt_json_finish(const void* target) noexcept
{
    return invoke_checked([&]
    {
        json_finish(argument<json_writer>(target));
    });
}

extern "C" int txrt_json_close_reader(const void* target) noexcept
{
    return invoke_checked([&]
    {
        json_close(argument<json_reader>(target));
    });
}

extern "C" int txrt_json_close_writer(const void* target) noexcept
{
    return invoke_checked([&]
    {
        json_close(argument<json_writer>(target));
    });
}

extern "C" int txrt_json_validate(const void* data, const void* schema) noexcept
{
    return invoke_checked([&]
    {
        json_validate(value(data), argument<tx_dict>(schema));
    });
}

extern "C" int txrt_json_read_binary(const void* source, const void* limits, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(json_read(
            format_read_source(argument<binary_stream>(source)), read_limits(limits)));
    });
}

extern "C" int txrt_json_reader_binary(const void* source, const void* limits, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<binary_stream>(source);
        *result = make_handle<std::any>(json_new_reader(
            format_read_source(stream), read_limits(limits), format_stream_guard(stream)));
    });
}

extern "C" int txrt_json_write_binary(const void* target, const void* data, const void* limits) noexcept
{
    return invoke_checked([&]
    {
        json_write(format_write_sink(argument<binary_stream>(target)), value(data), read_limits(limits));
    });
}

extern "C" int txrt_json_writer_binary(const void* target, const void* limits, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<binary_stream>(target);
        *result = make_handle<std::any>(json_new_writer(format_write_sink(stream), [stream]
        {
            stream_flush(stream);
        }, read_limits(limits)));
    });
}

extern "C" int txrt_json_read_text(const void* source, const void* limits, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(json_read(
            format_read_source(argument<text_stream>(source)), read_limits(limits)));
    });
}

extern "C" int txrt_json_reader_text(const void* source, const void* limits, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<text_stream>(source);
        *result = make_handle<std::any>(json_new_reader(
            format_read_source(stream), read_limits(limits), format_stream_guard(stream)));
    });
}

extern "C" int txrt_json_write_text(const void* target, const void* data, const void* limits) noexcept
{
    return invoke_checked([&]
    {
        json_write(format_write_sink(argument<text_stream>(target)), value(data), read_limits(limits));
    });
}

extern "C" int txrt_json_writer_text(const void* target, const void* limits, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<text_stream>(target);
        *result = make_handle<std::any>(json_new_writer(format_write_sink(stream), [stream]
        {
            stream_flush(stream);
        }, read_limits(limits)));
    });
}
