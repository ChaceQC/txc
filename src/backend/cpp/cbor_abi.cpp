#include "backend/cpp/cbor_abi.hpp"
#include "backend/cpp/format_abi.hpp"
#include "stdlib/cbor.hpp"
#include "stdlib/file_stream.hpp"

namespace
{

tx_generated::cbor_limits read_limits(const void* value)
{
    using tx_generated::format_abi::member;
    return {member<std::int64_t>(value, 0), member<std::int64_t>(value, 1),
        member<std::int64_t>(value, 2), member<std::int64_t>(value, 3)};
}

} // namespace

using namespace tx_generated;
using namespace tx_generated::format_abi;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_cbor_default_limits(const char* type, void** result) noexcept
{
    return invoke_checked([&]
    {
        const cbor_limits limits;
        *result = structure(type, "limits", {{"max_bytes", limits.max_bytes},
            {"max_value_bytes", limits.max_value_bytes},
            {"max_depth", limits.max_depth}, {"max_items", limits.max_items}});
    });
}

extern "C" int txrt_cbor_encode(const void* data, const void* limits,
                                void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(cbor_encode(value(data), read_limits(limits)));
    });
}

extern "C" int txrt_cbor_decode(const void* data, const void* limits,
                                void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(cbor_decode(
            argument<byte_value>(data), read_limits(limits)));
    });
}

extern "C" int txrt_cbor_read(const void* source, const void* limits,
                              void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(cbor_read(format_read_source(
            argument<binary_stream>(source)), read_limits(limits)));
    });
}

extern "C" int txrt_cbor_write(const void* target, const void* data,
                               const void* limits) noexcept
{
    return invoke_checked([&]
    {
        cbor_write(format_write_sink(argument<binary_stream>(target)),
                   value(data), read_limits(limits));
    });
}

extern "C" int txrt_cbor_reader(const void* source, const void* limits,
                                void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<binary_stream>(source);
        *result = make_handle<std::any>(cbor_new_reader(format_read_source(stream),
            read_limits(limits), format_stream_guard(stream)));
    });
}

extern "C" int txrt_cbor_next(const void* reader, const char* type,
                              void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = option(type, cbor_next(argument<cbor_reader>(reader)));
    });
}

extern "C" int txrt_cbor_writer(const void* target, std::int64_t count,
                                const void* limits, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<binary_stream>(target);
        *result = make_handle<std::any>(cbor_new_writer(format_write_sink(stream),
            [stream]
            {
                stream_flush(stream);
            }, count, read_limits(limits)));
    });
}

extern "C" int txrt_cbor_write_value(const void* writer, const void* data) noexcept
{
    return invoke_checked([&]
    {
        cbor_write_value(argument<cbor_writer>(writer), value(data));
    });
}

extern "C" int txrt_cbor_finish(const void* writer) noexcept
{
    return invoke_checked([&]
    {
        cbor_finish(argument<cbor_writer>(writer));
    });
}

extern "C" int txrt_cbor_close_reader(const void* reader) noexcept
{
    return invoke_checked([&]
    {
        cbor_close(argument<cbor_reader>(reader));
    });
}

extern "C" int txrt_cbor_close_writer(const void* writer) noexcept
{
    return invoke_checked([&]
    {
        cbor_close(argument<cbor_writer>(writer));
    });
}
