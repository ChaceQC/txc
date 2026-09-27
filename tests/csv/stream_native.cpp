#include "stdlib/csv.hpp"
#include "stdlib/error.hpp"

#include <iostream>
#include <stdexcept>

using namespace tx_generated;

void expect(bool condition)
{
    if (!condition)
    {
        throw std::runtime_error("CSV assertion failed");
    }
}

template<class operation>
void expect_error(const char* code, operation apply)
{
    try
    {
        apply();
    }
    catch (const runtime_failure& error)
    {
        if (error.error().code != code)
        {
            throw std::runtime_error(std::string("CSV expected ") + code + ", got " + error.error().code);
        }
        return;
    }
    throw std::runtime_error(std::string("CSV expected error ") + code);
}

format_source chunks(std::string text, std::size_t width)
{
    return [text = std::move(text), width, offset = std::size_t{0}]() mutable
    {
        const auto result = text.substr(offset, width);
        offset += result.size();
        return result;
    };
}

void check_chunks()
{
    csv_dialect dialect;
    dialect.has_header = true;
    dialect.allow_bom = true;
    const std::string text = "\xef\xbb\xbf姓名,备注,空\r\n甲𝄞,\"第一行\r\n第二行,\"\"引用\"\"\",\r\n乙,最后,尾";
    for (const auto width : {1, 2, 7, 4096})
    {
        auto reader = csv_new_reader(chunks(text, width), dialect);
        expect(csv_header(reader) == csv_row({"姓名", "备注", "空"}));
        expect(csv_next_row(reader) == csv_row({"甲𝄞", "第一行\r\n第二行,\"引用\"", ""}));
        expect(csv_next_row(reader) == csv_row({"乙", "最后", "尾"}));
        expect(!csv_next_row(reader));
        expect(!csv_next_row(reader));
    }
    expect(csv_parse("", {}).empty());
    expect(csv_parse("\r\n", {}) == std::vector<csv_row>({{""}}));
    expect(csv_parse(",", {}) == std::vector<csv_row>({{"", ""}}));
    expect(csv_parse("\"\"", {}) == std::vector<csv_row>({{""}}));
    expect(csv_parse("a,b,\r\n", {}) == std::vector<csv_row>({{"a", "b", ""}}));
    std::vector<csv_row> rows = {{"", "\xef\xbb\xbf", "文", "a,b"},
        {"\"", "\n", "\r\n", std::string(12000, 'x')}};
    expect(csv_parse(csv_stringify(rows, {}), {}) == rows);
    csv_dialect custom;
    custom.delimiter = ";";
    custom.quote = "'";
    custom.newline = "auto";
    expect(csv_parse("'a;b';'it''s'\nx;y\r\n", custom) ==
        std::vector<csv_row>({{"a;b", "it's"}, {"x", "y"}}));
    expect(csv_parse(csv_stringify(rows, custom), custom) == rows);
}

void check_errors()
{
    for (const auto text : {"\"unterminated", "a\"b", "\"a\"x", "a\rb", "a\n"})
    {
        expect_error("invalid_syntax", [&]
        {
            (void)csv_parse(text, {});
        });
    }
    expect_error("unexpected_bom", []
    {
        (void)csv_parse("\xef\xbb\xbf", {});
    });
    expect_error("invalid_utf8", []
    {
        (void)csv_parse("\"\xf0\x80\x80\x80\"", {});
    });
    expect_error("row_width", []
    {
        (void)csv_parse("a,b\r\nc", {});
    });
    csv_dialect dialect;
    dialect.has_header = true;
    expect_error("missing_header", [&]
    {
        (void)csv_parse("", dialect);
    });
    for (const auto text : {"a,a\r\n", "a,\r\n"})
    {
        expect_error("invalid_header", [&]
        {
            (void)csv_parse(text, dialect);
        });
    }
    dialect.has_header = false;
    dialect.strict_width = false;
    expect(csv_parse("a,b\r\nc", dialect).size() == 2);
    dialect.delimiter = "\n";
    expect_error("invalid_argument", [&]
    {
        (void)csv_parse("", dialect);
    });
    auto reader = csv_new_reader(chunks("\"中文\"x", 1), {});
    try
    {
        (void)csv_next_row(reader);
        expect(false);
    }
    catch (const runtime_failure& error)
    {
        expect(error.error().message.find("第 1 行第 5 列（字节偏移 8）") != std::string::npos);
    }
    expect_error("invalid_state", [&]
    {
        (void)csv_next_row(reader);
    });
}

void check_limits()
{
    for (const auto member : {&csv_dialect::max_field_bytes, &csv_dialect::max_row_bytes,
        &csv_dialect::max_columns, &csv_dialect::max_bytes, &csv_dialect::max_rows})
    {
        csv_dialect dialect;
        dialect.*member = 1;
        expect_error("size_limit", [&]
        {
            (void)csv_parse("ab,cd\r\nef,gh\r\n", dialect);
        });
        expect_error("size_limit", [&]
        {
            (void)csv_stringify({{"ab", "cd"}, {"ef", "gh"}}, dialect);
        });
    }
    csv_dialect dialect;
    dialect.max_field_bytes = 2;
    dialect.max_row_bytes = 5;
    dialect.max_bytes = 5;
    expect(csv_parse("ab,x\n", [&]
    {
        auto copy = dialect;
        copy.newline = "\n";
        return copy;
    }()).size() == 1);
    expect_error("size_limit", [&]
    {
        (void)csv_parse("\"a\"\"b\"", dialect);
    });
    expect_error("row_width", []
    {
        (void)csv_stringify({{}}, {});
    });
}

void check_resources()
{
    auto owner = std::make_shared<int>(0);
    std::weak_ptr<int> weak = owner;
    auto reader = csv_new_reader([owner]
    {
        return ++*owner == 1 ? std::string("a\r\n") : std::string{};
    }, {});
    owner.reset();
    expect(csv_next_row(reader) == csv_row({"a"}));
    csv_close(reader);
    csv_close(reader);
    expect(weak.expired());
    expect_error("invalid_state", [&]
    {
        (void)csv_next_row(reader);
    });
    std::string partial;
    auto writer = csv_new_writer([&](std::string_view part)
    {
        partial += part.substr(0, 1);
        throw runtime_failure({tx::error_kind::io, "operation_failed", "injected write failure"});
    }, []
    {
    }, {});
    expect_error("operation_failed", [&]
    {
        csv_write_row(writer, {"abc"});
    });
    expect(partial == "a");
    expect_error("invalid_state", [&]
    {
        csv_finish(writer);
    });
    csv_close(writer);
    csv_dialect header;
    header.has_header = true;
    expect_error("missing_header", [&]
    {
        (void)csv_stringify({}, header);
    });
}

void check_large_stream()
{
    int generated = 0;
    csv_dialect dialect;
    dialect.max_field_bytes = 8;
    dialect.max_row_bytes = 16;
    auto reader = csv_new_reader([&]
    {
        return generated++ < 20000 ? std::string("value,x\r\n") : std::string{};
    }, dialect);
    expect(generated == 1);
    int count = 0;
    while (const auto row = csv_next_row(reader))
    {
        expect(*row == csv_row({"value", "x"}));
        ++count;
    }
    expect(count == 20000);
}

int main()
{
    check_chunks();
    check_errors();
    check_limits();
    check_resources();
    check_large_stream();
    std::cout << "CSV_STREAM_NATIVE_OK\n";
}
