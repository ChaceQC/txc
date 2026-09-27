#include "stdlib/xml.hpp"

#include <libxml/xmlwriter.h>
#include <libxml/xmlsave.h>

#include <exception>
#include <unordered_set>
#include <utility>

namespace tx_generated
{

struct xml_writer_state
{
    format_sink sink;
    std::function<void()> flush;
    xml_limits limits;
    xmlTextWriterPtr raw = nullptr;
    std::exception_ptr callback_error;
    std::uint64_t written = 0;
    std::int64_t nodes = 0;
    std::vector<std::unordered_set<std::string>> attributes;
    std::vector<std::string> default_namespaces;
    bool root_seen = false;
    bool finished = false;
    bool failed = false;

    ~xml_writer_state()
    {
        if (raw)
        {
            xmlFreeTextWriter(raw);
        }
    }
};

namespace
{

struct output_state
{
    format_sink sink;
    std::uint64_t limit = 0;
    std::uint64_t written = 0;
    std::exception_ptr error;
};

int write_output(void* context, const char* buffer, int length)
{
    auto& state = *static_cast<output_state*>(context);
    try
    {
        if (length < 0 || static_cast<std::uint64_t>(length) >
            state.limit - state.written)
        {
            xml_runtime_failure("size_limit", "XML 输出超过总字节上限");
        }
        state.sink(std::string_view(buffer, static_cast<std::size_t>(length)));
        state.written += length;
        return length;
    }
    catch (...)
    {
        state.error = std::current_exception();
        return -1;
    }
}

int write_writer(void* context, const char* buffer, int length)
{
    auto& state = *static_cast<xml_writer_state*>(context);
    try
    {
        if (length < 0 || static_cast<std::uint64_t>(length) >
            static_cast<std::uint64_t>(state.limits.max_bytes) - state.written)
        {
            xml_runtime_failure("size_limit", "XML 输出超过总字节上限");
        }
        state.sink(std::string_view(buffer, static_cast<std::size_t>(length)));
        state.written += length;
        return length;
    }
    catch (...)
    {
        state.callback_error = std::current_exception();
        return -1;
    }
}

int close_output(void*)
{
    return 0;
}

void require_writer(const xml_writer& writer)
{
    if (!writer || !writer->raw || writer->failed || writer->finished)
    {
        xml_runtime_failure("invalid_state", "XML writer 已关闭、结束或因先前错误失效");
    }
}

void check_status(const xml_writer& writer, int status)
{
    if (writer->callback_error)
    {
        std::rethrow_exception(writer->callback_error);
    }
    if (status < 0)
    {
        xml_runtime_failure("operation_failed", "XML 写入失败");
    }
}

template<class operation>
void mutate(const xml_writer& writer, operation&& run)
{
    require_writer(writer);
    try
    {
        run();
    }
    catch (...)
    {
        writer->failed = true;
        throw;
    }
}

const xmlChar* xml_name(const std::string& value)
{
    return BAD_CAST value.c_str();
}

} // namespace

void xml_write(format_sink sink, const xml_document& document,
               const xml_limits& limits, bool omit_declaration)
{
    xml_validate_tree(document, limits);
    output_state output{std::move(sink), static_cast<std::uint64_t>(limits.max_bytes),
        0, {}};
    auto* save = xmlSaveToIO(write_output, close_output, &output, "UTF-8",
        omit_declaration ? XML_SAVE_NO_DECL : 0);
    if (!save)
    {
        xml_runtime_failure("allocation_failed", "无法创建 XML 输出器");
    }
    const auto written = xmlSaveDoc(save, document->raw);
    const int closed = xmlSaveClose(save);
    if (output.error)
    {
        std::rethrow_exception(output.error);
    }
    if (written < 0 || closed < 0)
    {
        xml_runtime_failure("operation_failed", "无法序列化 XML 文档");
    }
}

std::string xml_stringify(const xml_document& document, const xml_limits& limits)
{
    std::string result;
    xml_write([&](std::string_view chunk)
    {
        result.append(chunk);
    }, document, limits);
    return result;
}

xml_writer xml_new_writer(format_sink sink, std::function<void()> flush,
                          const xml_limits& limits, bool text_output)
{
    xml_check_limits(limits);
    auto result = std::make_shared<xml_writer_state>();
    result->sink = std::move(sink);
    result->flush = std::move(flush);
    result->limits = limits;
    auto* buffer = xmlOutputBufferCreateIO(write_writer, close_output,
        result.get(), nullptr);
    if (!buffer)
    {
        xml_runtime_failure("allocation_failed", "无法创建 XML 输出缓冲");
    }
    result->raw = xmlNewTextWriter(buffer);
    if (!result->raw)
    {
        xmlOutputBufferClose(buffer);
        xml_runtime_failure("allocation_failed", "无法创建 XML writer");
    }
    check_status(result, xmlTextWriterStartDocument(result->raw, "1.0",
        text_output ? nullptr : "UTF-8", nullptr));
    return result;
}

void xml_start_element(const xml_writer& writer, std::string_view local_name,
                       std::string_view uri, std::string_view prefix)
{
    mutate(writer, [&]
    {
        xml_check_name(local_name, uri, prefix, false);
        if (writer->attributes.empty() && writer->root_seen)
        {
            xml_runtime_failure("invalid_state", "XML 只能有一个根元素");
        }
        if (writer->attributes.size() >= static_cast<std::size_t>(writer->limits.max_depth))
        {
            xml_runtime_failure("depth_limit", "XML 嵌套深度超过上限");
        }
        if (++writer->nodes > writer->limits.max_nodes)
        {
            xml_runtime_failure("size_limit", "XML 节点数超过上限");
        }
        const std::string name(local_name);
        const std::string ns(uri);
        const std::string short_name(prefix);
        const bool reset_default = ns.empty() && short_name.empty() &&
            !writer->default_namespaces.empty() &&
            !writer->default_namespaces.back().empty();
        const int status = ns.empty()
            ? xmlTextWriterStartElement(writer->raw, xml_name(name))
            : xmlTextWriterStartElementNS(writer->raw,
                short_name.empty() ? nullptr : xml_name(short_name),
                xml_name(name), xml_name(ns));
        check_status(writer, status);
        if (reset_default)
        {
            check_status(writer, xmlTextWriterWriteAttribute(writer->raw,
                BAD_CAST "xmlns", BAD_CAST ""));
        }
        writer->attributes.emplace_back();
        const std::string inherited = writer->default_namespaces.empty()
            ? "" : writer->default_namespaces.back();
        writer->default_namespaces.push_back(short_name.empty() ? ns : inherited);
        writer->root_seen = true;
    });
}

void xml_write_attribute(const xml_writer& writer, std::string_view local_name,
                         std::string_view uri, std::string_view prefix,
                         std::string_view value)
{
    mutate(writer, [&]
    {
        xml_check_name(local_name, uri, prefix, true);
        xml_check_text(value);
        if (writer->attributes.empty())
        {
            xml_runtime_failure("invalid_state", "XML 属性必须位于元素起始标签内");
        }
        if (value.size() > static_cast<std::size_t>(writer->limits.max_text_bytes))
        {
            xml_runtime_failure("size_limit", "XML 属性值超过上限");
        }
        auto& names = writer->attributes.back();
        if (names.size() >= static_cast<std::size_t>(writer->limits.max_attributes))
        {
            xml_runtime_failure("size_limit", "XML 属性数超过上限");
        }
        const std::string key = std::string(uri) + '\0' + std::string(local_name);
        if (!names.insert(key).second)
        {
            xml_runtime_failure("duplicate_attribute", "XML 属性重复");
        }
        const std::string name(local_name);
        const std::string ns(uri);
        const std::string short_name(prefix);
        const std::string content(value);
        const int status = ns.empty()
            ? xmlTextWriterWriteAttribute(writer->raw, xml_name(name), xml_name(content))
            : xmlTextWriterWriteAttributeNS(writer->raw, xml_name(short_name),
                xml_name(name), xml_name(ns), xml_name(content));
        check_status(writer, status);
    });
}

void xml_write_text(const xml_writer& writer, std::string_view value)
{
    mutate(writer, [&]
    {
        xml_check_text(value);
        if (writer->attributes.empty())
        {
            xml_runtime_failure("invalid_state", "XML 文本必须位于元素内");
        }
        if (value.size() > static_cast<std::size_t>(writer->limits.max_text_bytes) ||
            ++writer->nodes > writer->limits.max_nodes)
        {
            xml_runtime_failure("size_limit", "XML 文本或节点数超过上限");
        }
        const std::string content(value);
        check_status(writer, xmlTextWriterWriteString(writer->raw, xml_name(content)));
    });
}

void xml_end_element(const xml_writer& writer)
{
    mutate(writer, [&]
    {
        if (writer->attributes.empty())
        {
            xml_runtime_failure("invalid_state", "XML 没有待关闭的元素");
        }
        check_status(writer, xmlTextWriterEndElement(writer->raw));
        writer->attributes.pop_back();
        writer->default_namespaces.pop_back();
    });
}

void xml_finish(const xml_writer& writer)
{
    if (writer && writer->finished)
    {
        return;
    }
    mutate(writer, [&]
    {
        if (!writer->root_seen || !writer->attributes.empty())
        {
            xml_runtime_failure("invalid_state", "XML 文档缺少根元素或标签未闭合");
        }
        check_status(writer, xmlTextWriterEndDocument(writer->raw));
        check_status(writer, xmlTextWriterFlush(writer->raw));
        if (writer->flush)
        {
            writer->flush();
        }
        xmlFreeTextWriter(writer->raw);
        writer->raw = nullptr;
        if (writer->callback_error)
        {
            std::rethrow_exception(writer->callback_error);
        }
        writer->finished = true;
    });
}

void xml_close(const xml_writer& writer)
{
    if (writer)
    {
        if (writer->raw)
        {
            xmlFreeTextWriter(writer->raw);
            writer->raw = nullptr;
        }
        writer->sink = {};
        writer->flush = {};
    }
}

} // namespace tx_generated
