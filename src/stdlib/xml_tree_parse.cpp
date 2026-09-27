#include "stdlib/xml.hpp"

#include <libxml/parser.h>
#include <libxml/xmlerror.h>

#include <algorithm>
#include <limits>
#include <utility>

namespace tx_generated
{
namespace
{

void check_tree(xmlDocPtr document, const xml_limits& limits)
{
    if (document->intSubset || document->extSubset)
    {
        xml_parse_failure("forbidden_dtd", "禁止 DTD 和自定义实体");
    }
    auto* root = xmlDocGetRootElement(document);
    if (!root)
    {
        xml_parse_failure("invalid_syntax", "XML 缺少根元素");
    }
    std::vector<std::pair<xmlNodePtr, std::int64_t>> pending;
    for (auto* child = document->children; child; child = child->next)
    {
        if (child->type != XML_DTD_NODE)
        {
            pending.emplace_back(child, child->type == XML_ELEMENT_NODE ? 1 : 0);
        }
    }
    std::int64_t nodes = 0;
    while (!pending.empty())
    {
        const auto [node, depth] = pending.back();
        pending.pop_back();
        if (node->type == XML_ENTITY_REF_NODE || node->type == XML_ENTITY_NODE)
        {
            xml_parse_failure("forbidden_dtd", "禁止自定义实体", node->line);
        }
        if (++nodes > limits.max_nodes)
        {
            xml_parse_failure("size_limit", "节点数超过上限", node->line);
        }
        if (node->type == XML_ELEMENT_NODE)
        {
            if (depth > limits.max_depth)
            {
                xml_parse_failure("depth_limit", "嵌套深度超过上限", node->line);
            }
            std::int64_t attributes = 0;
            for (auto* item = node->properties; item; item = item->next)
            {
                if (++attributes > limits.max_attributes)
                {
                    xml_parse_failure("size_limit", "属性数超过上限", node->line);
                }
                auto* content = xmlNodeListGetString(document, item->children, 1);
                const std::size_t size = content ? xmlStrlen(content) : 0;
                if (content)
                {
                    xmlFree(content);
                }
                if (size > static_cast<std::size_t>(limits.max_text_bytes))
                {
                    xml_parse_failure("size_limit", "属性值超过上限", node->line);
                }
            }
        }
        else if ((node->type == XML_TEXT_NODE || node->type == XML_CDATA_SECTION_NODE ||
                  node->type == XML_COMMENT_NODE || node->type == XML_PI_NODE) &&
                 node->content && xmlStrlen(node->content) > limits.max_text_bytes)
        {
            xml_parse_failure("size_limit", "文本节点超过上限", node->line);
        }
        for (auto* child = node->children; child; child = child->next)
        {
            pending.emplace_back(child,
                depth + (child->type == XML_ELEMENT_NODE ? 1 : 0));
        }
    }
}

xml_document parse_bytes(std::string_view input, const xml_limits& limits,
                         const char* encoding)
{
    xml_check_limits(limits);
    if (limits.max_bytes > 67108864)
    {
        xml_runtime_failure("invalid_argument", "整树解析的 max_bytes 至多为 64 MiB");
    }
    if (input.size() > static_cast<std::size_t>(limits.max_bytes))
    {
        xml_parse_failure("size_limit", "输入超过总字节上限");
    }
    if (input.empty())
    {
        xml_parse_failure("empty_input", "XML 文本不能为空");
    }
    xmlResetLastError();
    auto result = std::make_shared<xml_document_state>();
    result->raw = xmlReadMemory(input.data(), static_cast<int>(input.size()),
        nullptr, encoding, XML_PARSE_NONET | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    if (!result->raw)
    {
        const auto* error = xmlGetLastError();
        const auto line = error && error->line > 0 ? error->line : 1;
        const auto column = error && error->int2 > 0 ? error->int2 : 1;
        xml_parse_failure(xml_parse_code(error ? error->code : 0),
            "XML 语法或编码无效", line, column);
    }
    check_tree(result->raw, limits);
    return result;
}

} // namespace

xml_document xml_parse(std::string_view text, const xml_limits& limits)
{
    return parse_bytes(text, limits, "UTF-8");
}

xml_document xml_read(format_source source, const xml_limits& limits,
                      bool decoded_text)
{
    xml_check_limits(limits);
    if (limits.max_bytes > 67108864)
    {
        xml_runtime_failure("invalid_argument", "整树读取的 max_bytes 至多为 64 MiB");
    }
    std::string input;
    while (true)
    {
        auto chunk = source();
        if (chunk.empty())
        {
            break;
        }
        if (chunk.size() > static_cast<std::size_t>(limits.max_bytes) - input.size())
        {
            xml_parse_failure("size_limit", "输入超过总字节上限", 1, 1,
                input.size());
        }
        input += chunk;
    }
    return parse_bytes(input, limits, decoded_text ? "UTF-8" : nullptr);
}

void xml_validate_tree(const xml_document& document, const xml_limits& limits)
{
    xml_check_limits(limits);
    if (!document || !document->raw)
    {
        xml_runtime_failure("invalid_state", "XML 文档不可用");
    }
    check_tree(document->raw, limits);
}

} // namespace tx_generated
