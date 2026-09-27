#pragma once

#include "stdlib/format_stream.hpp"
#include "stdlib/json.hpp"

#include <memory>
#include <optional>

namespace tx_generated
{

struct json_limits
{
    std::int64_t max_bytes = 1073741824;
    std::int64_t max_value_bytes = 16777216;
    std::int64_t max_depth = 128;
};

struct json_reader_state;
struct json_writer_state;
using json_reader = std::shared_ptr<json_reader_state>;
using json_writer = std::shared_ptr<json_writer_state>;

void json_check_limits(const json_limits& limits);
std::any json_read(format_source source, const json_limits& limits);
json_reader json_new_reader(format_source source, const json_limits& limits,
                            std::function<void()> guard = {});
std::optional<std::any> json_next(const json_reader& reader);
void json_close(const json_reader& reader);
void json_write(format_sink sink, const std::any& value, const json_limits& limits);
json_writer json_new_writer(format_sink sink, std::function<void()> flush,
                            const json_limits& limits);
void json_write_value(const json_writer& writer, const std::any& value);
void json_finish(const json_writer& writer);
void json_close(const json_writer& writer);
void json_emit(const std::any& value, format_sink sink, std::uint64_t max_bytes,
               std::size_t max_depth, std::size_t start_depth = 0,
               std::size_t max_nodes = SIZE_MAX);
void json_validate(const std::any& value, const tx_dict& schema);

} // namespace tx_generated
