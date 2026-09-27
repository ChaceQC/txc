#pragma once

#include <cstdint>

extern "C"
{

int txrt_xml_default_limits(const char* type, void** result) noexcept;
int txrt_xml_reader_binary(const void* source, const void* limits, void** result) noexcept;
int txrt_xml_reader_text(const void* source, const void* limits, void** result) noexcept;
int txrt_xml_next(const void* reader, const char* option_type,
                  const char* event_type, void** result) noexcept;
int txrt_xml_attribute_count_reader(const void* reader, std::int64_t* result) noexcept;
int txrt_xml_attribute_local_name_reader(const void* reader, std::int64_t index,
                                         void** result) noexcept;
int txrt_xml_attribute_namespace_uri_reader(const void* reader, std::int64_t index,
                                             void** result) noexcept;
int txrt_xml_attribute_prefix_reader(const void* reader, std::int64_t index,
                                     void** result) noexcept;
int txrt_xml_attribute_value_reader(const void* reader, std::int64_t index,
                                    void** result) noexcept;
int txrt_xml_close_reader(const void* reader) noexcept;

int txrt_xml_parse(const void* text, const void* limits, void** result) noexcept;
int txrt_xml_read_binary(const void* source, const void* limits, void** result) noexcept;
int txrt_xml_read_text(const void* source, const void* limits, void** result) noexcept;
int txrt_xml_root(const void* document, void** result) noexcept;
int txrt_xml_first_child(const void* node, const char* type, void** result) noexcept;
int txrt_xml_next_sibling(const void* node, const char* type, void** result) noexcept;
int txrt_xml_kind(const void* node, void** result) noexcept;
int txrt_xml_local_name(const void* node, void** result) noexcept;
int txrt_xml_namespace_uri(const void* node, void** result) noexcept;
int txrt_xml_prefix(const void* node, void** result) noexcept;
int txrt_xml_text(const void* node, void** result) noexcept;
int txrt_xml_attribute_count_node(const void* node, std::int64_t* result) noexcept;
int txrt_xml_attribute_local_name_node(const void* node, std::int64_t index,
                                       void** result) noexcept;
int txrt_xml_attribute_namespace_uri_node(const void* node, std::int64_t index,
                                           void** result) noexcept;
int txrt_xml_attribute_prefix_node(const void* node, std::int64_t index,
                                   void** result) noexcept;
int txrt_xml_attribute_value_node(const void* node, std::int64_t index,
                                  void** result) noexcept;
int txrt_xml_new_document(const void* name, const void* uri, const void* prefix,
                          void** result) noexcept;
int txrt_xml_append_element(const void* parent, const void* name, const void* uri,
                            const void* prefix, void** result) noexcept;
int txrt_xml_set_attribute(const void* node, const void* name, const void* uri,
                           const void* prefix, const void* value) noexcept;
int txrt_xml_append_text(const void* node, const void* value) noexcept;
int txrt_xml_stringify(const void* document, const void* limits, void** result) noexcept;
int txrt_xml_write_binary(const void* target, const void* document,
                          const void* limits) noexcept;
int txrt_xml_write_text_stream(const void* target, const void* document,
                               const void* limits) noexcept;

int txrt_xml_writer_binary(const void* target, const void* limits, void** result) noexcept;
int txrt_xml_writer_text(const void* target, const void* limits, void** result) noexcept;
int txrt_xml_start_element(const void* writer, const void* name, const void* uri,
                           const void* prefix) noexcept;
int txrt_xml_write_attribute(const void* writer, const void* name, const void* uri,
                             const void* prefix, const void* value) noexcept;
int txrt_xml_write_text(const void* writer, const void* value) noexcept;
int txrt_xml_end_element(const void* writer) noexcept;
int txrt_xml_finish(const void* writer) noexcept;
int txrt_xml_close_writer(const void* writer) noexcept;

}
