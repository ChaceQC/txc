#include "stdlib/csv.hpp"
#include "stdlib/error.hpp"
#include "stdlib/json_utf8.hpp"

namespace tx_generated
{

struct csv_reader_state
{
    std::unique_ptr<format_input> input;
    std::function<void()> guard;
    csv_dialect dialect;
    csv_row header;
    std::size_t width = 0;
    std::int64_t rows = 0;
    bool failed = false;
};

namespace
{

void require_reader(const csv_reader& reader)
{
    if (!reader || !reader->input || reader->failed)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_state",
            "CSV reader 已关闭或因先前错误失效"});
    }
}

void append_scalar(format_input& input, std::string& field, std::size_t limit)
{
    const auto lead = input.peek();
    if (lead == 0)
    {
        input.fail("invalid_syntax", "CSV 字段不能包含 NUL");
    }
    const std::size_t width = lead < 0x80 ? 1 : lead < 0xe0 ? 2 : lead < 0xf0 ? 3 : 4;
    std::string scalar;
    for (std::size_t index = 0; index < width && input.peek(index) >= 0; ++index)
    {
        scalar += static_cast<char>(input.peek(index));
    }
    if (scalar.empty() || json_detail::utf8_width(scalar, 0) == 0)
    {
        input.fail("invalid_utf8", "CSV 字段包含无效 UTF-8");
    }
    if (width > limit - field.size())
    {
        input.fail("size_limit", "CSV 字段超过字节上限");
    }
    for (std::size_t index = 0; index < width; ++index)
    {
        field += input.get();
    }
}

std::string read_field(format_input& input, const csv_dialect& dialect)
{
    std::string field;
    const char quote = dialect.quote[0];
    const bool quoted = input.take(quote);
    while (true)
    {
        const auto byte = input.peek();
        if (byte < 0)
        {
            if (quoted)
            {
                input.fail("invalid_syntax", "CSV 引号字段缺少闭引号");
            }
            return field;
        }
        if (quoted && input.take(quote))
        {
            if (input.peek() != quote)
            {
                return field;
            }
            // 双引号占两个输入字节，但只生成一个字段字符。
            append_scalar(input, field, dialect.max_field_bytes);
        }
        else if (!quoted && (byte == dialect.delimiter[0] || byte == '\r' || byte == '\n'))
        {
            return field;
        }
        else
        {
            if (!quoted && byte == quote)
            {
                input.fail("invalid_syntax", "CSV 非引号字段中不能出现引号");
            }
            append_scalar(input, field, dialect.max_field_bytes);
        }
    }
}

void consume_newline(format_input& input, const csv_dialect& dialect)
{
    if (input.peek() == '\r')
    {
        if (dialect.newline == "\n")
        {
            input.fail("invalid_syntax", "CSV 行结束符不符合 LF dialect");
        }
        (void)input.get();
        if (!input.take('\n'))
        {
            input.fail("invalid_syntax", "CSV 引号外的 CR 后需要 LF");
        }
    }
    else if (input.peek() == '\n')
    {
        if (dialect.newline == "\r\n")
        {
            input.fail("invalid_syntax", "CSV 行结束符不符合 CRLF dialect");
        }
        (void)input.get();
    }
    else if (input.peek() >= 0)
    {
        input.fail("invalid_syntax", "CSV 闭引号后需要分隔符或行结束符");
    }
}

std::optional<csv_row> read_record(csv_reader_state& state)
{
    auto& input = *state.input;
    const auto& dialect = state.dialect;
    if (input.peek() < 0)
    {
        return std::nullopt;
    }
    if (state.rows >= dialect.max_rows)
    {
        input.fail("size_limit", "CSV 超过记录数上限");
    }
    input.begin_value(dialect.max_row_bytes);
    csv_row row;
    while (true)
    {
        if (row.size() >= static_cast<std::size_t>(dialect.max_columns))
        {
            input.fail("size_limit", "CSV 超过列数上限");
        }
        row.push_back(read_field(input, dialect));
        if (!input.take(dialect.delimiter[0]))
        {
            consume_newline(input, dialect);
            break;
        }
    }
    input.end_value();
    if (state.rows != 0 && (dialect.has_header || dialect.strict_width) && row.size() != state.width)
    {
        input.fail("row_width", "CSV 行宽与首行或表头不一致");
    }
    state.width = row.size();
    ++state.rows;
    return row;
}

} // namespace

csv_reader csv_new_reader(format_source source, const csv_dialect& dialect,
                          std::function<void()> guard)
{
    csv_check_dialect(dialect);
    auto reader = std::make_shared<csv_reader_state>();
    reader->dialect = dialect;
    reader->guard = std::move(guard);
    reader->input = std::make_unique<format_input>(std::move(source), "CSV", dialect.max_bytes);
    auto& input = *reader->input;
    if (input.peek() == 0xef && input.peek(1) == 0xbb && input.peek(2) == 0xbf)
    {
        if (!dialect.allow_bom)
        {
            input.fail("unexpected_bom", "CSV 未启用 UTF-8 BOM");
        }
        for (int index = 0; index < 3; ++index)
        {
            (void)input.get();
        }
    }
    if (dialect.has_header)
    {
        auto row = read_record(*reader);
        if (!row)
        {
            input.fail("missing_header", "CSV 缺少表头");
        }
        if (!csv_valid_header(*row))
        {
            input.fail("invalid_header", "CSV 表头列名不能为空或重复");
        }
        reader->header = std::move(*row);
    }
    return reader;
}

std::optional<csv_row> csv_next_row(const csv_reader& reader)
{
    require_reader(reader);
    try
    {
        if (reader->guard)
        {
            reader->guard();
        }
        return read_record(*reader);
    }
    catch (...)
    {
        reader->failed = true;
        throw;
    }
}

csv_row csv_header(const csv_reader& reader)
{
    require_reader(reader);
    return reader->header;
}

void csv_close(const csv_reader& reader)
{
    if (reader)
    {
        reader->input.reset();
        reader->guard = {};
        reader->header.clear();
    }
}

} // namespace tx_generated
