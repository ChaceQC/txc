#include "stdlib/xml.hpp"

#include <libxml/xmlreader.h>
#include <libxml/xmlerror.h>

#include <algorithm>
#include <cstring>
#include <exception>
#include <utility>

namespace tx_generated
{

struct xml_reader_state
{
    format_source source;
    std::function<void()> guard;
    xml_limits limits;
    xmlTextReaderPtr raw = nullptr;
    std::string pending;
    std::size_t pending_offset = 0;
    std::uint64_t input_bytes = 0;
    std::int64_t nodes = 0;
    std::vector<xml_attribute_data> attributes;
    std::exception_ptr callback_error;
    bool current_element = false;
    bool root_seen = false;
    bool finished = false;
    bool failed = false;

    ~xml_reader_state()
    {
        if (raw)
        {
            xmlFreeTextReader(raw);
        }
    }
};

namespace
{

std::string copy_xml(const xmlChar* value)
{
    return value ? reinterpret_cast<const char*>(value) : "";
}

int read_callback(void* context, char* buffer, int size)
{
    auto& state = *static_cast<xml_reader_state*>(context);
    try
    {
        if (state.pending_offset == state.pending.size())
        {
            state.pending = state.source();
            state.pending_offset = 0;
        }
        if (state.pending.empty())
        {
            return 0;
        }
        const auto count = std::min<std::size_t>(size,
            state.pending.size() - state.pending_offset);
        if (count > static_cast<std::uint64_t>(state.limits.max_bytes) -
                state.input_bytes)
        {
            xml_parse_failure("size_limit", "输入超过总字节上限", 1, 1,
                state.input_bytes);
        }
        std::memcpy(buffer, state.pending.data() + state.pending_offset, count);
        state.pending_offset += count;
        state.input_bytes += count;
        return static_cast<int>(count);
    }
    catch (...)
    {
        state.callback_error = std::current_exception();
        return -1;
    }
}

int close_callback(void*)
{
    return 0;
}

std::string event_kind(int type)
{
    switch (type)
    {
    case XML_READER_TYPE_ELEMENT: return "start_element";
    case XML_READER_TYPE_END_ELEMENT: return "end_element";
    case XML_READER_TYPE_TEXT:
    case XML_READER_TYPE_WHITESPACE:
    case XML_READER_TYPE_SIGNIFICANT_WHITESPACE: return "text";
    case XML_READER_TYPE_CDATA: return "cdata";
    case XML_READER_TYPE_COMMENT: return "comment";
    case XML_READER_TYPE_PROCESSING_INSTRUCTION: return "pi";
    default: return "";
    }
}

std::int64_t byte_offset(const xml_reader_state& state)
{
    const auto consumed = xmlTextReaderByteConsumed(state.raw);
    return consumed >= 0 ? consumed : static_cast<std::int64_t>(state.input_bytes);
}

void collect_attributes(xml_reader_state& state, std::int64_t line,
                        std::int64_t column)
{
    if (xmlTextReaderMoveToFirstAttribute(state.raw) != 1)
    {
        return;
    }
    do
    {
        if (xmlTextReaderIsNamespaceDecl(state.raw) == 1)
        {
            continue;
        }
        if (state.attributes.size() >= static_cast<std::size_t>(state.limits.max_attributes))
        {
            xml_parse_failure("size_limit", "属性数超过上限", line, column,
                byte_offset(state));
        }
        xml_attribute_data item{copy_xml(xmlTextReaderConstLocalName(state.raw)),
            copy_xml(xmlTextReaderConstNamespaceUri(state.raw)),
            copy_xml(xmlTextReaderConstPrefix(state.raw)),
            copy_xml(xmlTextReaderConstValue(state.raw))};
        if (item.value.size() > static_cast<std::size_t>(state.limits.max_text_bytes))
        {
            xml_parse_failure("size_limit", "属性值超过上限", line, column,
                byte_offset(state));
        }
        state.attributes.push_back(std::move(item));
    } while (xmlTextReaderMoveToNextAttribute(state.raw) == 1);
    (void)xmlTextReaderMoveToElement(state.raw);
}

[[noreturn]] void reader_invalid()
{
    xml_runtime_failure("invalid_state", "XML reader 已关闭或因先前错误失效");
}

} // namespace

xml_reader xml_new_reader(format_source source, const xml_limits& limits,
                          std::function<void()> guard, bool decoded_text)
{
    xml_check_limits(limits);
    auto result = std::make_shared<xml_reader_state>();
    result->source = std::move(source);
    result->guard = std::move(guard);
    result->limits = limits;
    result->raw = xmlReaderForIO(read_callback, close_callback, result.get(),
        nullptr, decoded_text ? "UTF-8" : nullptr,
        XML_PARSE_NONET | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    if (result->callback_error)
    {
        std::rethrow_exception(result->callback_error);
    }
    if (!result->raw)
    {
        xml_parse_failure("invalid_syntax", "无法创建 XML reader");
    }
    xmlResetLastError();
    (void)xmlTextReaderSetParserProp(result->raw, XML_PARSER_LOADDTD, 0);
    (void)xmlTextReaderSetParserProp(result->raw, XML_PARSER_SUBST_ENTITIES, 0);
    (void)xmlTextReaderSetParserProp(result->raw, XML_PARSER_VALIDATE, 0);
    return result;
}

std::optional<xml_event_data> xml_next(const xml_reader& reader)
{
    if (!reader || !reader->raw || reader->failed)
    {
        reader_invalid();
    }
    try
    {
        if (reader->guard)
        {
            reader->guard();
        }
        if (reader->finished)
        {
            return std::nullopt;
        }
        reader->attributes.clear();
        reader->current_element = false;
        while (true)
        {
            const int status = xmlTextReaderRead(reader->raw);
            if (reader->callback_error)
            {
                std::rethrow_exception(reader->callback_error);
            }
            if (status == 0)
            {
                if (!reader->root_seen)
                {
                    xml_parse_failure("empty_input", "XML 缺少根元素");
                }
                reader->finished = true;
                return std::nullopt;
            }
            const auto line = std::max(1, xmlTextReaderGetParserLineNumber(reader->raw));
            const auto column = std::max(1, xmlTextReaderGetParserColumnNumber(reader->raw));
            if (status < 0)
            {
                const auto* error = xmlGetLastError();
                xml_parse_failure(xml_parse_code(error ? error->code : 0),
                    "XML 语法或编码无效", line,
                    column, byte_offset(*reader));
            }
            const int type = xmlTextReaderNodeType(reader->raw);
            if (type == XML_READER_TYPE_DOCUMENT_TYPE ||
                type == XML_READER_TYPE_ENTITY_REFERENCE ||
                type == XML_READER_TYPE_ENTITY)
            {
                xml_parse_failure("forbidden_dtd", "禁止 DTD 和自定义实体", line,
                    column, byte_offset(*reader));
            }
            auto kind = event_kind(type);
            if (kind.empty())
            {
                continue;
            }
            const auto depth = xmlTextReaderDepth(reader->raw);
            const auto element_depth = depth +
                (type == XML_READER_TYPE_ELEMENT ||
                 type == XML_READER_TYPE_END_ELEMENT ? 1 : 0);
            if (element_depth > reader->limits.max_depth)
            {
                xml_parse_failure("depth_limit", "嵌套深度超过上限", line,
                    column, byte_offset(*reader));
            }
            if (kind != "end_element" && ++reader->nodes > reader->limits.max_nodes)
            {
                xml_parse_failure("size_limit", "节点数超过上限", line,
                    column, byte_offset(*reader));
            }
            xml_event_data event{std::move(kind), depth,
                copy_xml(xmlTextReaderConstLocalName(reader->raw)),
                copy_xml(xmlTextReaderConstNamespaceUri(reader->raw)),
                copy_xml(xmlTextReaderConstPrefix(reader->raw)),
                copy_xml(xmlTextReaderConstValue(reader->raw)),
                xmlTextReaderIsEmptyElement(reader->raw) == 1, line, column};
            if (event.text.size() > static_cast<std::size_t>(reader->limits.max_text_bytes))
            {
                xml_parse_failure("size_limit", "文本节点超过上限", line,
                    column, byte_offset(*reader));
            }
            reader->current_element = event.kind == "start_element";
            if (reader->current_element)
            {
                reader->root_seen |= depth == 0;
                collect_attributes(*reader, line, column);
            }
            return event;
        }
    }
    catch (...)
    {
        reader->failed = true;
        throw;
    }
}

std::int64_t xml_reader_attribute_count(const xml_reader& reader)
{
    if (!reader || !reader->raw || reader->failed || !reader->current_element)
    {
        reader_invalid();
    }
    return static_cast<std::int64_t>(reader->attributes.size());
}

xml_attribute_data xml_reader_attribute(const xml_reader& reader, std::int64_t index)
{
    const auto count = xml_reader_attribute_count(reader);
    if (index < 0 || index >= count)
    {
        xml_runtime_failure("out_of_range", "XML 属性索引越界");
    }
    return reader->attributes[static_cast<std::size_t>(index)];
}

void xml_close(const xml_reader& reader)
{
    if (reader)
    {
        if (reader->raw)
        {
            xmlFreeTextReader(reader->raw);
            reader->raw = nullptr;
        }
        reader->source = {};
        reader->guard = {};
        reader->attributes.clear();
    }
}

} // namespace tx_generated
