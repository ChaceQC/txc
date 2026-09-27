#include "stdlib/xml.hpp"

#include <libxml/parser.h>
#include <libxml/xmlerror.h>

namespace tx_generated
{

xml_document_state::~xml_document_state()
{
    if (raw)
    {
        xmlFreeDoc(raw);
    }
}

void xml_check_limits(const xml_limits& limits)
{
    if (limits.max_bytes <= 0 || limits.max_bytes > 1099511627776LL ||
        limits.max_depth <= 0 || limits.max_depth > 256 ||
        limits.max_nodes <= 0 || limits.max_nodes > 10000000 ||
        limits.max_text_bytes <= 0 || limits.max_text_bytes > 8388608 ||
        limits.max_attributes <= 0 || limits.max_attributes > 65536)
    {
        xml_runtime_failure("invalid_argument", "XML 限额无效");
    }
}

void xml_check_name(std::string_view local_name, std::string_view uri,
                    std::string_view prefix, bool attribute)
{
    if (local_name.find('\0') != std::string_view::npos ||
        uri.find('\0') != std::string_view::npos ||
        prefix.find('\0') != std::string_view::npos)
    {
        xml_runtime_failure("invalid_name", "XML 名称或命名空间包含 NUL");
    }
    xml_check_text(uri);
    const std::string name(local_name);
    const std::string requested_prefix(prefix);
    constexpr std::string_view xml_uri = "http://www.w3.org/XML/1998/namespace";
    if (name.empty() || xmlValidateNCName(BAD_CAST name.c_str(), 0) != 0 ||
        (!prefix.empty() && xmlValidateNCName(BAD_CAST requested_prefix.c_str(), 0) != 0) ||
        (uri.empty() && !prefix.empty()) ||
        (attribute && !uri.empty() && prefix.empty()) ||
        prefix == "xmlns" || (uri == xml_uri) != (prefix == "xml") ||
        uri == "http://www.w3.org/2000/xmlns/" ||
        (local_name == "xmlns" && prefix.empty()))
    {
        xml_runtime_failure("invalid_name", "XML 名称或命名空间组合无效");
    }
}

void xml_check_text(std::string_view value)
{
    for (std::size_t index = 0; index < value.size();)
    {
        const auto lead = static_cast<unsigned char>(value[index]);
        std::uint32_t point = 0;
        std::size_t length = 0;
        if (lead < 0x80)
        {
            point = lead;
            length = 1;
        }
        else if (lead >= 0xc2 && lead <= 0xdf)
        {
            point = lead & 0x1f;
            length = 2;
        }
        else if (lead >= 0xe0 && lead <= 0xef)
        {
            point = lead & 0x0f;
            length = 3;
        }
        else if (lead >= 0xf0 && lead <= 0xf4)
        {
            point = lead & 0x07;
            length = 4;
        }
        if (!length || length > value.size() - index)
        {
            xml_runtime_failure("invalid_argument", "XML 文本含非法 UTF-8");
        }
        for (std::size_t offset = 1; offset < length; ++offset)
        {
            const auto byte = static_cast<unsigned char>(value[index + offset]);
            if ((byte & 0xc0) != 0x80)
            {
                xml_runtime_failure("invalid_argument", "XML 文本含非法 UTF-8");
            }
            point = (point << 6) | (byte & 0x3f);
        }
        if ((length == 2 && point < 0x80) ||
            (length == 3 && point < 0x800) ||
            (length == 4 && point < 0x10000) ||
            !(point == 9 || point == 10 || point == 13 ||
              (point >= 0x20 && point <= 0xd7ff) ||
              (point >= 0xe000 && point <= 0xfffd) ||
              (point >= 0x10000 && point <= 0x10ffff)))
        {
            xml_runtime_failure("invalid_argument", "XML 文本含非法字符");
        }
        index += length;
    }
}

[[noreturn]] void xml_parse_failure(const char* code, std::string_view detail,
                                    std::int64_t line, std::int64_t column,
                                    std::int64_t byte_offset)
{
    throw runtime_failure({tx::error_kind::parse, code,
        "XML 第 " + std::to_string(line) + " 行第 " + std::to_string(column) +
        " 列" + (byte_offset >= 0
            ? "（字节偏移 " + std::to_string(byte_offset) + "）" : "") + "：" +
        std::string(detail)});
}

const char* xml_parse_code(int parser_code)
{
    switch (parser_code)
    {
    case XML_ERR_DOCUMENT_EMPTY: return "empty_input";
    case XML_ERR_UNKNOWN_ENCODING:
    case XML_ERR_UNSUPPORTED_ENCODING:
    case XML_ERR_INVALID_ENCODING: return "invalid_encoding";
    default: return "invalid_syntax";
    }
}

[[noreturn]] void xml_runtime_failure(const char* code, std::string_view detail)
{
    throw runtime_failure({tx::error_kind::runtime, code, std::string(detail)});
}

} // namespace tx_generated
