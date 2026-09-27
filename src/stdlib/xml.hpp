#pragma once

#include "stdlib/format_stream.hpp"
#include "stdlib/error.hpp"

#include <libxml/tree.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tx_generated
{

struct xml_limits
{
    std::int64_t max_bytes = 1073741824;
    std::int64_t max_depth = 128;
    std::int64_t max_nodes = 1000000;
    std::int64_t max_text_bytes = 8388608;
    std::int64_t max_attributes = 4096;
};

struct xml_attribute_data
{
    std::string local_name;
    std::string namespace_uri;
    std::string prefix;
    std::string value;
};

struct xml_event_data
{
    std::string kind;
    std::int64_t depth = 0;
    std::string local_name;
    std::string namespace_uri;
    std::string prefix;
    std::string text;
    bool empty_element = false;
    std::int64_t line = 0;
    std::int64_t column = 0;
};

struct xml_document_state
{
    xmlDocPtr raw = nullptr;
    ~xml_document_state();
};

using xml_document = std::shared_ptr<xml_document_state>;

struct xml_node_state
{
    xml_document document;
    xmlNodePtr raw = nullptr;
};

using xml_node = std::shared_ptr<xml_node_state>;
struct xml_reader_state;
struct xml_writer_state;
using xml_reader = std::shared_ptr<xml_reader_state>;
using xml_writer = std::shared_ptr<xml_writer_state>;

void xml_check_limits(const xml_limits& limits);
void xml_check_name(std::string_view local_name, std::string_view uri,
                    std::string_view prefix, bool attribute);
void xml_check_text(std::string_view value);
const char* xml_parse_code(int parser_code);
[[noreturn]] void xml_parse_failure(const char* code, std::string_view detail,
                                    std::int64_t line = 1, std::int64_t column = 1,
                                    std::int64_t byte_offset = -1);
[[noreturn]] void xml_runtime_failure(const char* code, std::string_view detail);

xml_reader xml_new_reader(format_source source, const xml_limits& limits,
                          std::function<void()> guard = {}, bool decoded_text = false);
std::optional<xml_event_data> xml_next(const xml_reader& reader);
std::int64_t xml_reader_attribute_count(const xml_reader& reader);
xml_attribute_data xml_reader_attribute(const xml_reader& reader, std::int64_t index);
void xml_close(const xml_reader& reader);

xml_document xml_parse(std::string_view text, const xml_limits& limits);
xml_document xml_read(format_source source, const xml_limits& limits,
                      bool decoded_text = false);
void xml_validate_tree(const xml_document& document, const xml_limits& limits);
xml_node xml_root(const xml_document& document);
std::optional<xml_node> xml_first_child(const xml_node& node);
std::optional<xml_node> xml_next_sibling(const xml_node& node);
std::string xml_node_kind(const xml_node& node);
std::string xml_node_local_name(const xml_node& node);
std::string xml_node_namespace_uri(const xml_node& node);
std::string xml_node_prefix(const xml_node& node);
std::string xml_node_text(const xml_node& node);
std::int64_t xml_node_attribute_count(const xml_node& node);
xml_attribute_data xml_node_attribute(const xml_node& node, std::int64_t index);
xml_document xml_new_document(std::string_view local_name, std::string_view uri,
                              std::string_view prefix);
xml_node xml_append_element(const xml_node& parent, std::string_view local_name,
                             std::string_view uri, std::string_view prefix);
void xml_set_attribute(const xml_node& node, std::string_view local_name,
                       std::string_view uri, std::string_view prefix,
                       std::string_view value);
void xml_append_text(const xml_node& node, std::string_view value);
void xml_write(format_sink sink, const xml_document& document, const xml_limits& limits,
               bool omit_declaration = false);
std::string xml_stringify(const xml_document& document, const xml_limits& limits);

xml_writer xml_new_writer(format_sink sink, std::function<void()> flush,
                          const xml_limits& limits, bool text_output = false);
void xml_start_element(const xml_writer& writer, std::string_view local_name,
                       std::string_view uri, std::string_view prefix);
void xml_write_attribute(const xml_writer& writer, std::string_view local_name,
                         std::string_view uri, std::string_view prefix,
                         std::string_view value);
void xml_write_text(const xml_writer& writer, std::string_view value);
void xml_end_element(const xml_writer& writer);
void xml_finish(const xml_writer& writer);
void xml_close(const xml_writer& writer);

} // namespace tx_generated
