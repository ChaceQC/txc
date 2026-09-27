#include "backend/cpp/xml_abi.hpp"
#include "backend/cpp/xml_abi_internal.hpp"

using namespace tx_generated;
using namespace tx_generated::format_abi;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

namespace
{

int attribute_text(const void* source, std::int64_t index, void** result,
                   std::string xml_attribute_data::* field) noexcept
{
    return invoke_checked([&]
    {
        const auto attribute = xml_reader_attribute(argument<xml_reader>(source), index);
        *result = make_handle<std::string>(attribute.*field);
    });
}

} // namespace

extern "C" int txrt_xml_default_limits(const char* type, void** result) noexcept
{
    return invoke_checked([&]
    {
        const xml_limits limits;
        *result = structure(type, "limits", {{"max_bytes", limits.max_bytes},
            {"max_depth", limits.max_depth}, {"max_nodes", limits.max_nodes},
            {"max_text_bytes", limits.max_text_bytes},
            {"max_attributes", limits.max_attributes}});
    });
}

extern "C" int txrt_xml_reader_binary(const void* source, const void* limits,
                                       void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<binary_stream>(source);
        *result = make_handle<std::any>(xml_new_reader(format_read_source(stream),
            xml_abi::read_limits(limits), format_stream_guard(stream)));
    });
}

extern "C" int txrt_xml_reader_text(const void* source, const void* limits,
                                     void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<text_stream>(source);
        *result = make_handle<std::any>(xml_new_reader(format_read_source(stream),
            xml_abi::read_limits(limits), format_stream_guard(stream), true));
    });
}

extern "C" int txrt_xml_next(const void* source, const char* option_type,
                               const char* event_type, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto event = xml_next(argument<xml_reader>(source));
        std::optional<std::any> content;
        if (event)
        {
            auto* data = static_cast<std::any*>(structure(event_type, "event", {
                {"kind", event->kind}, {"depth", event->depth},
                {"local_name", event->local_name},
                {"namespace_uri", event->namespace_uri}, {"prefix", event->prefix},
                {"text", event->text}, {"empty_element", event->empty_element},
                {"line", event->line}, {"column", event->column}}));
            content = std::move(*data);
            detail::destroy_handle(data);
        }
        *result = option(option_type, std::move(content));
    });
}

extern "C" int txrt_xml_attribute_count_reader(const void* source,
                                                 std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = xml_reader_attribute_count(argument<xml_reader>(source));
    });
}

extern "C" int txrt_xml_attribute_local_name_reader(const void* source,
    std::int64_t index, void** result) noexcept
{
    return attribute_text(source, index, result, &xml_attribute_data::local_name);
}

extern "C" int txrt_xml_attribute_namespace_uri_reader(const void* source,
    std::int64_t index, void** result) noexcept
{
    return attribute_text(source, index, result, &xml_attribute_data::namespace_uri);
}

extern "C" int txrt_xml_attribute_prefix_reader(const void* source,
    std::int64_t index, void** result) noexcept
{
    return attribute_text(source, index, result, &xml_attribute_data::prefix);
}

extern "C" int txrt_xml_attribute_value_reader(const void* source,
    std::int64_t index, void** result) noexcept
{
    return attribute_text(source, index, result, &xml_attribute_data::value);
}

extern "C" int txrt_xml_close_reader(const void* source) noexcept
{
    return invoke_checked([&]
    {
        xml_close(argument<xml_reader>(source));
    });
}
