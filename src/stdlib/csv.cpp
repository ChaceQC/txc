#include "stdlib/csv.hpp"
#include "stdlib/error.hpp"

#include <algorithm>
#include <unordered_set>

namespace tx_generated
{

void csv_check_dialect(const csv_dialect& dialect)
{
    const auto ascii = [](const std::string& value, bool tab)
    {
        return value.size() == 1 && ((value[0] >= 0x20 && value[0] <= 0x7e) ||
            (tab && value[0] == '\t'));
    };
    if (!ascii(dialect.delimiter, true) || !ascii(dialect.quote, false) ||
        dialect.delimiter == dialect.quote ||
        (dialect.newline != "\r\n" && dialect.newline != "\n" && dialect.newline != "auto") ||
        dialect.max_field_bytes <= 0 || dialect.max_field_bytes > 16777216 ||
        dialect.max_row_bytes <= 0 || dialect.max_row_bytes > 67108864 ||
        dialect.max_columns <= 0 || dialect.max_columns > 65536 ||
        dialect.max_bytes <= 0 || dialect.max_bytes > 1099511627776LL ||
        dialect.max_rows <= 0 || dialect.max_rows > 1000000000)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_argument",
            "CSV 分隔符、引号、换行或限额配置无效"});
    }
}

bool csv_valid_header(const csv_row& row)
{
    std::unordered_set<std::string_view> names;
    for (const auto& name : row)
    {
        if (name.empty() || !names.insert(name).second)
        {
            return false;
        }
    }
    return !row.empty();
}

std::vector<csv_row> csv_parse(std::string_view text, const csv_dialect& dialect)
{
    // 整文本入口复用相同状态机；文本由调用栈持有，游标不逃逸。
    auto reader = csv_new_reader([text, offset = std::size_t{0}]() mutable
    {
        const auto count = std::min<std::size_t>(4096, text.size() - offset);
        std::string result(text.substr(offset, count));
        offset += count;
        return result;
    }, dialect);
    std::vector<csv_row> result;
    if (dialect.has_header)
    {
        result.push_back(csv_header(reader));
    }
    while (auto row = csv_next_row(reader))
    {
        result.push_back(std::move(*row));
    }
    return result;
}

std::string csv_stringify(const std::vector<csv_row>& rows, const csv_dialect& dialect)
{
    std::string result;
    auto writer = csv_new_writer([&](std::string_view part)
    {
        result += part;
    }, []
    {
    }, dialect);
    for (const auto& row : rows)
    {
        csv_write_row(writer, row);
    }
    csv_finish(writer);
    return result;
}

} // namespace tx_generated
