#pragma once

#include "stdlib/format_stream.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace tx_generated
{

struct csv_dialect
{
    std::string delimiter = ",";
    std::string quote = "\"";
    std::string newline = "\r\n";
    bool has_header = false;
    bool strict_width = true;
    bool allow_bom = false;
    bool write_bom = false;
    std::int64_t max_field_bytes = 1048576;
    std::int64_t max_row_bytes = 16777216;
    std::int64_t max_columns = 4096;
    std::int64_t max_bytes = 1073741824;
    std::int64_t max_rows = 1000000;
};

using csv_row = std::vector<std::string>;
struct csv_reader_state;
struct csv_writer_state;
using csv_reader = std::shared_ptr<csv_reader_state>;
using csv_writer = std::shared_ptr<csv_writer_state>;

void csv_check_dialect(const csv_dialect& dialect);
bool csv_valid_header(const csv_row& row);
csv_reader csv_new_reader(format_source source, const csv_dialect& dialect,
                          std::function<void()> guard = {});
std::optional<csv_row> csv_next_row(const csv_reader& reader);
csv_row csv_header(const csv_reader& reader);
void csv_close(const csv_reader& reader);
csv_writer csv_new_writer(format_sink sink, std::function<void()> flush,
                          const csv_dialect& dialect);
void csv_write_row(const csv_writer& writer, const csv_row& row);
void csv_finish(const csv_writer& writer);
void csv_close(const csv_writer& writer);
std::vector<csv_row> csv_parse(std::string_view text, const csv_dialect& dialect);
std::string csv_stringify(const std::vector<csv_row>& rows, const csv_dialect& dialect);

} // namespace tx_generated
