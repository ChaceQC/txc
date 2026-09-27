#include "stdlib/xml.hpp"

#include <libxml/parser.h>

#include <limits>
#include <utility>

namespace tx_generated
{
namespace
{

xml_node make_node(const xml_document& document, xmlNodePtr raw)
{
    return std::make_shared<xml_node_state>(xml_node_state{document, raw});
}

xmlNodePtr require_node(const xml_node& node)
{
    if (!node || !node->document || !node->document->raw || !node->raw)
    {
        xml_runtime_failure("invalid_state", "XML 节点不可用");
    }
    return node->raw;
}

std::string copy_xml(const xmlChar* value)
{
    return value ? reinterpret_cast<const char*>(value) : "";
}

xmlNsPtr namespace_for(xmlNodePtr node, std::string_view uri,
                       std::string_view prefix)
{
    if (uri.empty())
    {
        return nullptr;
    }
    const std::string name(prefix);
    const std::string href(uri);
    auto* existing = xmlSearchNs(node->doc, node,
        prefix.empty() ? nullptr : BAD_CAST name.c_str());
    if (existing && xmlStrEqual(existing->href, BAD_CAST href.c_str()))
    {
        return existing;
    }
    for (auto* local = node->nsDef; local; local = local->next)
    {
        if (xmlStrEqual(local->prefix,
            prefix.empty() ? nullptr : BAD_CAST name.c_str()))
        {
            xml_runtime_failure("invalid_name", "XML 节点的前缀已绑定其他 URI");
        }
    }
    auto* created = xmlNewNs(node, BAD_CAST href.c_str(),
        prefix.empty() ? nullptr : BAD_CAST name.c_str());
    if (!created)
    {
        xml_runtime_failure("allocation_failed", "无法创建 XML 命名空间");
    }
    return created;
}

xmlNodePtr visible_node(xmlNodePtr node)
{
    while (node && node->type != XML_ELEMENT_NODE &&
           node->type != XML_TEXT_NODE && node->type != XML_CDATA_SECTION_NODE &&
           node->type != XML_COMMENT_NODE && node->type != XML_PI_NODE)
    {
        node = node->next;
    }
    return node;
}

} // namespace

xml_node xml_root(const xml_document& document)
{
    if (!document || !document->raw)
    {
        xml_runtime_failure("invalid_state", "XML 文档不可用");
    }
    auto* raw = xmlDocGetRootElement(document->raw);
    if (!raw)
    {
        xml_runtime_failure("invalid_state", "XML 文档缺少根元素");
    }
    return make_node(document, raw);
}

std::optional<xml_node> xml_first_child(const xml_node& node)
{
    const auto* raw = require_node(node);
    auto* child = visible_node(raw->children);
    return child ? std::optional<xml_node>(make_node(node->document, child))
                 : std::nullopt;
}

std::optional<xml_node> xml_next_sibling(const xml_node& node)
{
    auto* sibling = visible_node(require_node(node)->next);
    return sibling ? std::optional<xml_node>(make_node(node->document, sibling))
                   : std::nullopt;
}

std::string xml_node_kind(const xml_node& node)
{
    switch (require_node(node)->type)
    {
    case XML_ELEMENT_NODE: return "element";
    case XML_TEXT_NODE: return "text";
    case XML_CDATA_SECTION_NODE: return "cdata";
    case XML_COMMENT_NODE: return "comment";
    case XML_PI_NODE: return "pi";
    default: return "other";
    }
}

std::string xml_node_local_name(const xml_node& node)
{
    const auto* raw = require_node(node);
    return raw->type == XML_ELEMENT_NODE || raw->type == XML_PI_NODE
        ? copy_xml(raw->name) : "";
}

std::string xml_node_namespace_uri(const xml_node& node)
{
    const auto* raw = require_node(node);
    return raw->ns ? copy_xml(raw->ns->href) : "";
}

std::string xml_node_prefix(const xml_node& node)
{
    const auto* raw = require_node(node);
    return raw->ns ? copy_xml(raw->ns->prefix) : "";
}

std::string xml_node_text(const xml_node& node)
{
    auto* value = xmlNodeGetContent(require_node(node));
    auto result = copy_xml(value);
    if (value)
    {
        xmlFree(value);
    }
    return result;
}

std::int64_t xml_node_attribute_count(const xml_node& node)
{
    std::int64_t count = 0;
    for (auto* item = require_node(node)->properties; item; item = item->next)
    {
        ++count;
    }
    return count;
}

xml_attribute_data xml_node_attribute(const xml_node& node, std::int64_t index)
{
    if (index < 0)
    {
        xml_runtime_failure("out_of_range", "XML 属性索引越界");
    }
    auto* raw = require_node(node);
    auto* item = raw->properties;
    while (item && index-- > 0)
    {
        item = item->next;
    }
    if (!item)
    {
        xml_runtime_failure("out_of_range", "XML 属性索引越界");
    }
    auto* value = xmlNodeListGetString(raw->doc, item->children, 1);
    xml_attribute_data result{copy_xml(item->name),
        item->ns ? copy_xml(item->ns->href) : "",
        item->ns ? copy_xml(item->ns->prefix) : "", copy_xml(value)};
    if (value)
    {
        xmlFree(value);
    }
    return result;
}

xml_document xml_new_document(std::string_view local_name, std::string_view uri,
                              std::string_view prefix)
{
    xml_check_name(local_name, uri, prefix, false);
    auto document = std::make_shared<xml_document_state>();
    document->raw = xmlNewDoc(BAD_CAST "1.0");
    if (!document->raw)
    {
        xml_runtime_failure("allocation_failed", "无法创建 XML 文档");
    }
    const std::string name(local_name);
    auto* root = xmlNewDocNode(document->raw, nullptr, BAD_CAST name.c_str(), nullptr);
    if (!root)
    {
        xml_runtime_failure("allocation_failed", "无法创建 XML 根元素");
    }
    xmlDocSetRootElement(document->raw, root);
    xmlSetNs(root, namespace_for(root, uri, prefix));
    return document;
}

xml_node xml_append_element(const xml_node& parent, std::string_view local_name,
                             std::string_view uri, std::string_view prefix)
{
    xml_check_name(local_name, uri, prefix, false);
    auto* raw = require_node(parent);
    if (raw->type != XML_ELEMENT_NODE)
    {
        xml_runtime_failure("invalid_state", "只能在 XML 元素下添加子元素");
    }
    const std::string name(local_name);
    auto* child = xmlNewNode(nullptr, BAD_CAST name.c_str());
    if (!child)
    {
        xml_runtime_failure("allocation_failed", "无法创建 XML 子元素");
    }
    xmlAddChild(raw, child);
    if (uri.empty())
    {
        // 父节点的默认命名空间不会自动赋给无命名空间的子节点。
        auto* inherited = xmlSearchNs(raw->doc, raw, nullptr);
        if (inherited && inherited->href && *inherited->href)
        {
            xmlSetNs(child, xmlNewNs(child, BAD_CAST "", nullptr));
        }
    }
    else
    {
        xmlSetNs(child, namespace_for(child, uri, prefix));
    }
    return make_node(parent->document, child);
}

void xml_set_attribute(const xml_node& node, std::string_view local_name,
                       std::string_view uri, std::string_view prefix,
                       std::string_view value)
{
    xml_check_name(local_name, uri, prefix, true);
    xml_check_text(value);
    auto* raw = require_node(node);
    if (raw->type != XML_ELEMENT_NODE)
    {
        xml_runtime_failure("invalid_state", "只能给 XML 元素设置属性");
    }
    const std::string name(local_name);
    const std::string content(value);
    auto* result = uri.empty()
        ? xmlSetProp(raw, BAD_CAST name.c_str(), BAD_CAST content.c_str())
        : xmlSetNsProp(raw, namespace_for(raw, uri, prefix),
            BAD_CAST name.c_str(), BAD_CAST content.c_str());
    if (!result)
    {
        xml_runtime_failure("allocation_failed", "无法设置 XML 属性");
    }
}

void xml_append_text(const xml_node& node, std::string_view value)
{
    xml_check_text(value);
    if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        xml_runtime_failure("size_limit", "XML 文本过长");
    }
    auto* raw = require_node(node);
    if (raw->type != XML_ELEMENT_NODE)
    {
        xml_runtime_failure("invalid_state", "只能在 XML 元素下添加文本");
    }
    const std::string content(value);
    auto* child = xmlNewTextLen(BAD_CAST content.data(),
        static_cast<int>(content.size()));
    if (!child)
    {
        xml_runtime_failure("allocation_failed", "无法创建 XML 文本节点");
    }
    xmlAddChild(raw, child);
}

} // namespace tx_generated
