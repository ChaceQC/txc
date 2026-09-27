#include "backend/cpp/xml_abi.hpp"
#include "backend/cpp/xml_abi_internal.hpp"

using namespace tx_generated;
using namespace tx_generated::format_abi;
using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

namespace
{

int node_text(const void* source, void** result,
              std::string (*read)(const xml_node&)) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(read(argument<xml_node>(source)));
    });
}

int attribute_text(const void* source, std::int64_t index, void** result,
                   std::string xml_attribute_data::* field) noexcept
{
    return invoke_checked([&]
    {
        const auto attribute = xml_node_attribute(argument<xml_node>(source), index);
        *result = make_handle<std::string>(attribute.*field);
    });
}

} // namespace

extern "C" int txrt_xml_parse(const void* source, const void* limits,
                               void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = xml_abi::document_handle(xml_parse(text(source),
            xml_abi::read_limits(limits)));
    });
}

extern "C" int txrt_xml_read_binary(const void* source, const void* limits,
                                     void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = xml_abi::document_handle(xml_read(
            format_read_source(argument<binary_stream>(source)),
            xml_abi::read_limits(limits)));
    });
}

extern "C" int txrt_xml_read_text(const void* source, const void* limits,
                                   void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = xml_abi::document_handle(xml_read(
            format_read_source(argument<text_stream>(source)),
            xml_abi::read_limits(limits), true));
    });
}

extern "C" int txrt_xml_root(const void* source, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = xml_abi::node_handle(xml_root(argument<xml_document>(source)));
    });
}

extern "C" int txrt_xml_first_child(const void* source, const char* type,
                                      void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto child = xml_first_child(argument<xml_node>(source));
        *result = option(type, child ? std::optional<std::any>(*child) : std::nullopt);
    });
}

extern "C" int txrt_xml_next_sibling(const void* source, const char* type,
                                       void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto sibling = xml_next_sibling(argument<xml_node>(source));
        *result = option(type, sibling ? std::optional<std::any>(*sibling) : std::nullopt);
    });
}

extern "C" int txrt_xml_kind(const void* source, void** result) noexcept
{
    return node_text(source, result, xml_node_kind);
}

extern "C" int txrt_xml_local_name(const void* source, void** result) noexcept
{
    return node_text(source, result, xml_node_local_name);
}

extern "C" int txrt_xml_namespace_uri(const void* source, void** result) noexcept
{
    return node_text(source, result, xml_node_namespace_uri);
}

extern "C" int txrt_xml_prefix(const void* source, void** result) noexcept
{
    return node_text(source, result, xml_node_prefix);
}

extern "C" int txrt_xml_text(const void* source, void** result) noexcept
{
    return node_text(source, result, xml_node_text);
}

extern "C" int txrt_xml_attribute_count_node(const void* source,
                                               std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = xml_node_attribute_count(argument<xml_node>(source));
    });
}

extern "C" int txrt_xml_attribute_local_name_node(const void* source,
    std::int64_t index, void** result) noexcept
{
    return attribute_text(source, index, result, &xml_attribute_data::local_name);
}

extern "C" int txrt_xml_attribute_namespace_uri_node(const void* source,
    std::int64_t index, void** result) noexcept
{
    return attribute_text(source, index, result, &xml_attribute_data::namespace_uri);
}

extern "C" int txrt_xml_attribute_prefix_node(const void* source,
    std::int64_t index, void** result) noexcept
{
    return attribute_text(source, index, result, &xml_attribute_data::prefix);
}

extern "C" int txrt_xml_attribute_value_node(const void* source,
    std::int64_t index, void** result) noexcept
{
    return attribute_text(source, index, result, &xml_attribute_data::value);
}

extern "C" int txrt_xml_new_document(const void* name, const void* uri,
    const void* prefix, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = xml_abi::document_handle(xml_new_document(text(name), text(uri),
            text(prefix)));
    });
}

extern "C" int txrt_xml_append_element(const void* parent, const void* name,
    const void* uri, const void* prefix, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = xml_abi::node_handle(xml_append_element(argument<xml_node>(parent),
            text(name), text(uri), text(prefix)));
    });
}

extern "C" int txrt_xml_set_attribute(const void* node, const void* name,
    const void* uri, const void* prefix, const void* value) noexcept
{
    return invoke_checked([&]
    {
        xml_set_attribute(argument<xml_node>(node), text(name), text(uri),
            text(prefix), text(value));
    });
}

extern "C" int txrt_xml_append_text(const void* node, const void* value) noexcept
{
    return invoke_checked([&]
    {
        xml_append_text(argument<xml_node>(node), text(value));
    });
}

extern "C" int txrt_xml_stringify(const void* source, const void* limits,
                                   void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(xml_stringify(
            argument<xml_document>(source), xml_abi::read_limits(limits)));
    });
}

extern "C" int txrt_xml_write_binary(const void* target, const void* document,
    const void* limits) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<binary_stream>(target);
        xml_write(format_write_sink(stream), argument<xml_document>(document),
            xml_abi::read_limits(limits));
        stream_flush(stream);
    });
}

extern "C" int txrt_xml_write_text_stream(const void* target, const void* document,
    const void* limits) noexcept
{
    return invoke_checked([&]
    {
        const auto stream = argument<text_stream>(target);
        xml_write(format_write_sink(stream), argument<xml_document>(document),
            xml_abi::read_limits(limits), true);
        stream_flush(stream);
    });
}
