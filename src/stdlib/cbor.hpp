#pragma once

#include "stdlib/bytes.hpp"
#include "stdlib/format_stream.hpp"

#include <any>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace tx_generated
{

struct cbor_limits
{
    std::int64_t max_bytes = 1073741824;
    std::int64_t max_value_bytes = 16777216;
    std::int64_t max_depth = 128;
    std::int64_t max_items = 1000000;
};

struct cbor_reader_state;
struct cbor_writer_state;
using cbor_reader = std::shared_ptr<cbor_reader_state>;
using cbor_writer = std::shared_ptr<cbor_writer_state>;

void cbor_check_limits(const cbor_limits& limits);
byte_value cbor_encode(const std::any& value, const cbor_limits& limits);
std::any cbor_decode(const byte_value& data, const cbor_limits& limits);
std::any cbor_read(format_source source, const cbor_limits& limits);
void cbor_write(format_sink sink, const std::any& value, const cbor_limits& limits);
cbor_reader cbor_new_reader(format_source source, const cbor_limits& limits,
                            std::function<void()> guard);
std::optional<std::any> cbor_next(const cbor_reader& reader);
cbor_writer cbor_new_writer(format_sink sink, std::function<void()> flush,
                            std::int64_t count, const cbor_limits& limits);
void cbor_write_value(const cbor_writer& writer, const std::any& value);
void cbor_finish(const cbor_writer& writer);
void cbor_close(const cbor_reader& reader);
void cbor_close(const cbor_writer& writer);

} // namespace tx_generated
