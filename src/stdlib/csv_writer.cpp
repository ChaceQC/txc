#include "stdlib/csv.hpp"
#include "stdlib/error.hpp"
#include "stdlib/json_utf8.hpp"

namespace tx_generated
{

struct csv_writer_state
{
    format_sink sink;
    std::function<void()> flush;
    csv_dialect dialect;
    std::size_t width = 0;
    std::uint64_t written = 0;
    std::int64_t rows = 0;
    bool finished = false;
    bool failed = false;
};

namespace
{

[[noreturn]] void writer_error(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::runtime, code, message});
}

void require_writer(const csv_writer& writer)
{
    if (!writer || !writer->sink || writer->failed)
    {
        writer_error("invalid_state", "CSV writer 已关闭或因先前错误失效");
    }
}

void append(std::string& output, std::string_view part, std::size_t limit)
{
    if (part.size() > limit - output.size())
    {
        writer_error("size_limit", "CSV 输出行超过字节上限");
    }
    output += part;
}

void append_field(std::string& output, const std::string& field, const csv_dialect& dialect)
{
    if (field.size() > static_cast<std::size_t>(dialect.max_field_bytes))
    {
        writer_error("size_limit", "CSV 输出字段超过字节上限");
    }
    for (std::size_t index = 0; index < field.size();)
    {
        if (field[index] == 0)
        {
            writer_error("invalid_syntax", "CSV 字段不能包含 NUL");
        }
        const auto width = json_detail::utf8_width(field, index);
        if (width == 0)
        {
            writer_error("invalid_utf8", "CSV 字段包含无效 UTF-8");
        }
        index += width;
    }
    const bool quoted = field.empty() || field.starts_with("\xef\xbb\xbf") ||
        field.find_first_of(dialect.delimiter + dialect.quote + "\r\n") != std::string::npos;
    if (quoted)
    {
        append(output, dialect.quote, dialect.max_row_bytes);
    }
    for (const auto& byte : field)
    {
        if (byte == dialect.quote[0])
        {
            append(output, dialect.quote, dialect.max_row_bytes);
        }
        append(output, std::string_view(&byte, 1), dialect.max_row_bytes);
    }
    if (quoted)
    {
        append(output, dialect.quote, dialect.max_row_bytes);
    }
}

std::string encode_row(const csv_row& row, const csv_writer_state& writer)
{
    const auto& dialect = writer.dialect;
    if (row.empty())
    {
        writer_error("row_width", "CSV 输出行至少需要一个字段");
    }
    if (row.size() > static_cast<std::size_t>(dialect.max_columns) || writer.rows >= dialect.max_rows)
    {
        writer_error("size_limit", "CSV 输出超过列数或记录数上限");
    }
    if (writer.rows != 0 && (dialect.has_header || dialect.strict_width) && row.size() != writer.width)
    {
        writer_error("row_width", "CSV 输出行宽与首行或表头不一致");
    }
    if (dialect.has_header && writer.rows == 0 && !csv_valid_header(row))
    {
        writer_error("invalid_header", "CSV 表头列名不能为空或重复");
    }
    std::string output;
    for (std::size_t index = 0; index < row.size(); ++index)
    {
        if (index != 0)
        {
            append(output, dialect.delimiter, dialect.max_row_bytes);
        }
        append_field(output, row[index], dialect);
    }
    append(output, dialect.newline == "auto" ? "\r\n" : dialect.newline, dialect.max_row_bytes);
    return output;
}

} // namespace

csv_writer csv_new_writer(format_sink sink, std::function<void()> flush,
                          const csv_dialect& dialect)
{
    csv_check_dialect(dialect);
    auto writer = std::make_shared<csv_writer_state>();
    writer->sink = std::move(sink);
    writer->flush = std::move(flush);
    writer->dialect = dialect;
    if (dialect.write_bom)
    {
        if (dialect.max_bytes < 3)
        {
            writer_error("size_limit", "CSV 字节限额不足以写入 BOM");
        }
        writer->sink("\xef\xbb\xbf");
        writer->written = 3;
    }
    return writer;
}

void csv_write_row(const csv_writer& writer, const csv_row& row)
{
    require_writer(writer);
    if (writer->finished)
    {
        writer_error("invalid_state", "CSV writer 已完成");
    }
    try
    {
        const auto output = encode_row(row, *writer);
        if (output.size() > static_cast<std::uint64_t>(writer->dialect.max_bytes) - writer->written)
        {
            writer_error("size_limit", "CSV 输出超过总字节上限");
        }
        writer->sink(output);
        writer->written += output.size();
        writer->width = row.size();
        ++writer->rows;
    }
    catch (...)
    {
        writer->failed = true;
        throw;
    }
}

void csv_finish(const csv_writer& writer)
{
    require_writer(writer);
    if (writer->finished)
    {
        return;
    }
    try
    {
        if (writer->dialect.has_header && writer->rows == 0)
        {
            writer_error("missing_header", "CSV 输出缺少必需表头");
        }
        writer->flush();
        writer->finished = true;
    }
    catch (...)
    {
        writer->failed = true;
        throw;
    }
}

void csv_close(const csv_writer& writer)
{
    if (writer)
    {
        writer->sink = {};
        writer->flush = {};
    }
}

} // namespace tx_generated
