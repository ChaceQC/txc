#pragma once

#include "stdlib/serde.hpp"
#include "stdlib/cbor_internal.hpp"
#include "stdlib/json_output.hpp"
#include "stdlib/json_parser.hpp"

namespace tx_generated
{

inline constexpr std::size_t serde_byte_limit = 16 * 1024 * 1024;
inline constexpr cbor_limits serde_cbor_limits{serde_byte_limit, serde_byte_limit, 128, 1000000};

// 逻辑深度保留原 serde 的结构体/option 计数，wire 深度用于格式自身的限额。
struct serde_depth
{
    std::size_t logical = 0;
    std::size_t wire = 0;
    serde_depth child(std::size_t logical_step = 1, std::size_t wire_step = 1) const;
    void check(bool encoding) const;
};

class serde_writer
{
public:
    explicit serde_writer(serde_format format);
    void integer(std::int64_t value);
    void floating(double value);
    void boolean(bool value);
    void text(std::string_view value);
    void bytes(const byte_value& value);
    void null();
    void begin(bool object, std::size_t size, serde_depth depth);
    void end(bool object);
    void separator(std::size_t index);
    void key(std::string_view name, std::int64_t number);
    void dynamic(const std::any& value, std::size_t depth);
    std::string finish();
    serde_format format;
    serde_active active;

private:
    json_detail::output_buffer output_;
};

struct serde_sequence
{
    bool object = false;
    std::size_t index = 0;
    std::uint64_t size = 0;
    std::string previous_key;
};

class serde_reader
{
public:
    serde_reader(serde_format format, std::string_view input);
    serde_sequence begin(bool object, serde_depth depth);
    bool next(serde_sequence& sequence);
    std::any key(serde_sequence& sequence, std::size_t depth);
    std::any scalar(std::size_t depth);
    bool take_null();
    void skip(std::size_t depth);
    void finish();
    serde_format format;

private:
    format_input input_;
    json_parser json_;
};

struct serde_codec
{
    void (*encode)(serde_writer&, const std::any&, const serde_type&, serde_depth);
    std::any (*decode)(serde_reader&, const serde_type&, serde_depth);
};

void serde_write_struct(serde_writer& writer, const std::any& value,
    const serde_schema& schema, serde_depth depth);
std::any serde_read_struct(serde_reader& reader, const serde_schema& schema,
    serde_depth depth);

} // namespace tx_generated

extern "C"
{
extern const tx_generated::serde_codec tx_serde_integer;
extern const tx_generated::serde_codec tx_serde_floating;
extern const tx_generated::serde_codec tx_serde_boolean;
extern const tx_generated::serde_codec tx_serde_text;
extern const tx_generated::serde_codec tx_serde_bytes;
extern const tx_generated::serde_codec tx_serde_option;
extern const tx_generated::serde_codec tx_serde_structure;
extern const tx_generated::serde_codec tx_serde_vector_integer;
extern const tx_generated::serde_codec tx_serde_vector_floating;
extern const tx_generated::serde_codec tx_serde_vector_boolean;
extern const tx_generated::serde_codec tx_serde_vector_text;
extern const tx_generated::serde_codec tx_serde_vector_bytes;
extern const tx_generated::serde_codec tx_serde_vector_object;
}
