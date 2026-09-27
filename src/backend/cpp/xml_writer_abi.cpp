#include "backend/cpp/xml_abi.hpp"
#include "backend/cpp/xml_abi_internal.hpp"

using namespace tx_generated;
using namespace tx_generated::format_abi;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_xml_writer_binary(const void* target, const void* limits,
                                       void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<binary_stream>(target);
        *result = make_handle<std::any>(xml_new_writer(format_write_sink(stream),
            [stream]
            {
                stream_flush(stream);
            }, xml_abi::read_limits(limits)));
    });
}

extern "C" int txrt_xml_writer_text(const void* target, const void* limits,
                                     void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<text_stream>(target);
        *result = make_handle<std::any>(xml_new_writer(format_write_sink(stream),
            [stream]
            {
                stream_flush(stream);
            }, xml_abi::read_limits(limits), true));
    });
}

extern "C" int txrt_xml_start_element(const void* target, const void* name,
    const void* uri, const void* prefix) noexcept
{
    return invoke_checked([&]
    {
        xml_start_element(argument<xml_writer>(target), text(name), text(uri),
            text(prefix));
    });
}

extern "C" int txrt_xml_write_attribute(const void* target, const void* name,
    const void* uri, const void* prefix, const void* value) noexcept
{
    return invoke_checked([&]
    {
        xml_write_attribute(argument<xml_writer>(target), text(name), text(uri),
            text(prefix), text(value));
    });
}

extern "C" int txrt_xml_write_text(const void* target, const void* value) noexcept
{
    return invoke_checked([&]
    {
        xml_write_text(argument<xml_writer>(target), text(value));
    });
}

extern "C" int txrt_xml_end_element(const void* target) noexcept
{
    return invoke_checked([&]
    {
        xml_end_element(argument<xml_writer>(target));
    });
}

extern "C" int txrt_xml_finish(const void* target) noexcept
{
    return invoke_checked([&]
    {
        xml_finish(argument<xml_writer>(target));
    });
}

extern "C" int txrt_xml_close_writer(const void* target) noexcept
{
    return invoke_checked([&]
    {
        xml_close(argument<xml_writer>(target));
    });
}
