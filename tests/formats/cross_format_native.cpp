#include "stdlib/array.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/cbor.hpp"
#include "stdlib/csv.hpp"
#include "stdlib/error.hpp"
#include "stdlib/json_stream.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace tx_generated;

namespace
{

void expect(bool condition, std::string_view description)
{
    if (!condition)
    {
        throw std::runtime_error(std::string(description));
    }
}

template<class operation>
void expect_failure(std::string_view code, std::string_view location,
                    operation apply)
{
    try
    {
        apply();
    }
    catch (const runtime_failure& error)
    {
        expect(error.error().code == code, "unexpected error code");
        expect(error.error().message.find(location) != std::string::npos,
               "missing error location");
        return;
    }
    throw std::runtime_error("expected format error");
}

format_source chunks(std::string source, std::size_t width)
{
    return [source = std::move(source), width, offset = std::size_t{0}]() mutable
    {
        auto result = source.substr(offset, width);
        offset += result.size();
        return result;
    };
}

void check_positions()
{
    expect_failure("invalid_syntax", "第 2 行第 6 列（字节偏移 11）", []
    {
        (void)json_read(chunks("[\n\"中文\",?]", 1), {});
    });
    expect_failure("invalid_syntax", "第 1 行第 5 列（字节偏移 8）", []
    {
        (void)csv_parse("\"中文\"x", {});
    });
    expect_failure("invalid_syntax", "字节偏移 2", []
    {
        (void)cbor_decode(bytes_from_hex("8201"), {});
    });
}

void check_cycles_and_limits()
{
    tx_array cyclic;
    cyclic.push_back(cyclic);
    expect_failure("cyclic_value", "循环", [&]
    {
        (void)json_stringify(cyclic);
    });
    expect_failure("cyclic_value", "循环", [&]
    {
        (void)cbor_encode(cyclic, {});
    });
    cyclic.clear();

    expect_failure("size_limit", "字节偏移", []
    {
        (void)json_read(chunks("[1234]", 1), {5, 5, 128});
    });
    expect_failure("depth_limit", "字节偏移", []
    {
        (void)json_read(chunks("[[1]]", 1), {100, 100, 1});
    });
    csv_dialect dialect;
    dialect.max_field_bytes = 2;
    expect_failure("size_limit", "字节偏移", [&]
    {
        (void)csv_parse("abc", dialect);
    });
    dialect.max_field_bytes = 10;
    dialect.max_rows = 1;
    expect_failure("size_limit", "字节偏移", [&]
    {
        (void)csv_parse("a\r\na\r\n", dialect);
    });
    expect_failure("size_limit", "字节偏移", []
    {
        // 长度头宣称 1 MiB；必须先拒绝，再尝试分配或读取内容。
        (void)cbor_decode(bytes_from_hex("5a00100000"), {100, 32, 128, 100});
    });
    expect_failure("depth_limit", "字节偏移", []
    {
        (void)cbor_decode(bytes_from_hex("818180"), {100, 100, 1, 100});
    });
}

void check_partial_writes()
{
    const auto io_error = []() -> void
    {
        throw runtime_failure({tx::error_kind::io, "operation_failed",
            "injected partial write"});
    };
    std::string json_prefix;
    int json_calls = 0;
    auto json_writer = json_new_writer([&](std::string_view part)
    {
        if (++json_calls == 3)
        {
            json_prefix += part.substr(0, 1);
            io_error();
        }
        json_prefix += part;
    }, []
    {
    }, {});
    json_write_value(json_writer, std::int64_t{1});
    expect_failure("operation_failed", "partial write", [&]
    {
        json_write_value(json_writer, std::int64_t{2});
    });
    expect(json_prefix == "[1,", "JSON prefix after partial write");
    expect_failure("invalid_state", "", [&]
    {
        json_finish(json_writer);
    });
    json_close(json_writer);

    std::string csv_prefix;
    int csv_calls = 0;
    auto csv_writer = csv_new_writer([&](std::string_view part)
    {
        if (++csv_calls == 2)
        {
            csv_prefix += part.substr(0, 1);
            io_error();
        }
        csv_prefix += part;
    }, []
    {
    }, {});
    csv_write_row(csv_writer, {"first"});
    expect_failure("operation_failed", "partial write", [&]
    {
        csv_write_row(csv_writer, {"second"});
    });
    expect(csv_prefix.starts_with("first\r\ns"),
           "CSV prefix after partial write");
    expect_failure("invalid_state", "", [&]
    {
        csv_finish(csv_writer);
    });
    csv_close(csv_writer);

    std::string cbor_prefix;
    auto cbor_writer = cbor_new_writer([&](std::string_view part)
    {
        cbor_prefix += part.substr(0, 1);
        io_error();
    }, []
    {
    }, 1, {20000, 20000, 128, 100});
    expect_failure("operation_failed", "partial write", [&]
    {
        cbor_write_value(cbor_writer, std::string(10000, 'x'));
    });
    expect(cbor_prefix.size() == 1, "CBOR prefix after partial write");
    expect_failure("invalid_state", "", [&]
    {
        cbor_finish(cbor_writer);
    });
    cbor_close(cbor_writer);
}

void check_large_cross_format_stream()
{
    constexpr std::int64_t count = 20000;
    std::string json_source = "[";
    for (std::int64_t index = 0; index < count; ++index)
    {
        if (index != 0)
        {
            json_source += ',';
        }
        json_source += std::to_string(index);
    }
    json_source += ']';
    auto json_reader = json_new_reader(chunks(json_source, 7),
                                       {200000, 16, 128});
    std::string cbor_data;
    auto cbor_writer = cbor_new_writer([&](std::string_view part)
    {
        cbor_data += part;
    }, []
    {
    }, count, {200000, 16, 128, count});
    std::string csv_data;
    auto csv_writer = csv_new_writer([&](std::string_view part)
    {
        csv_data += part;
    }, []
    {
    }, {});
    std::int64_t read_count = 0;
    while (auto item = json_next(json_reader))
    {
        cbor_write_value(cbor_writer, *item);
        csv_write_row(csv_writer,
                      {std::to_string(std::any_cast<std::int64_t>(*item))});
        ++read_count;
    }
    cbor_finish(cbor_writer);
    csv_finish(csv_writer);
    expect(read_count == count, "JSON to CBOR element count");
    expect(cbor_data.size() < json_source.size(), "CBOR compact encoding");

    auto cbor_reader = cbor_new_reader(chunks(cbor_data, 7),
                                       {200000, 16, 128, count}, {});
    std::int64_t sum = 0;
    while (auto item = cbor_next(cbor_reader))
    {
        sum += std::any_cast<std::int64_t>(*item);
    }
    expect(sum == count * (count - 1) / 2, "CBOR decoded checksum");

    auto csv_reader = csv_new_reader(chunks(csv_data, 7), {});
    std::int64_t csv_sum = 0;
    std::int64_t csv_count = 0;
    while (auto row = csv_next_row(csv_reader))
    {
        expect(row->size() == 1, "CSV row width");
        csv_sum += std::stoll(row->front());
        ++csv_count;
    }
    expect(csv_count == count && csv_sum == sum,
           "JSON to CSV stream roundtrip");
}

} // namespace

int main()
{
    check_positions();
    check_cycles_and_limits();
    check_partial_writes();
    check_large_cross_format_stream();
    std::cout << "FORMAT_CROSS_NATIVE_OK\n";
}
