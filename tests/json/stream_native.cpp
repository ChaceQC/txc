#include "stdlib/json_stream.hpp"
#include "stdlib/format_stream.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace tx_generated;

void expect(bool condition)
{
    if (!condition)
    {
        throw std::runtime_error("JSON stream assertion failed");
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
        expect(error.error().code == code);
        return;
    }
    throw std::runtime_error("expected JSON error");
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
    const std::string text = "[null,{\"a\":\"中文𝄞\\uD834\\uDD1E\",\"a\":\"末\"},-1.25e+2,[true,false]]";
    for (const auto width : {1, 2, 7, 4096})
    {
        auto reader = json_new_reader(chunks(text, width), {});
        tx_array values;
        while (auto item = json_next(reader))
        {
            values.push_back(*item);
        }
        expect(json_stringify(values) == json_stringify(json_parse(text)));
        expect(!json_next(reader));
        json_close(reader);
        expect_error("invalid_state", [&]
        {
            (void)json_next(reader);
        });
    }
    std::string large(20000, 'x');
    large += "中文𝄞";
    std::string output;
    json_emit(large, [&](std::string_view part)
    {
        expect(part.size() <= 4096);
        output += part;
    }, 30000, 128);
    expect(std::any_cast<std::string>(json_read(chunks(output, 1), {})) == large);
    expect_error("invalid_utf8", []
    {
        (void)json_read(chunks("\"\xf0\x80\x80\x80\"", 1), {});
    });
    expect_error("invalid_escape", []
    {
        (void)json_read(chunks("\"\\uD800\"", 1), {});
    });
}

void check_limits_and_state()
{
    for (const auto text : {"[1,]", "[1", "[1]x", "[01]", "[NaN]", "[1 2]"})
    {
        auto reader = json_new_reader(chunks(text, 1), {});
        expect_error("invalid_syntax", [&]
        {
            while (json_next(reader))
            {
            }
        });
        expect_error("invalid_state", [&]
        {
            (void)json_next(reader);
        });
    }
    expect_error("size_limit", []
    {
        (void)json_read(chunks("\"abcd\"", 1), {100, 5, 128});
    });
    expect_error("size_limit", []
    {
        (void)json_read(chunks("123 ", 1), {3, 3, 128});
    });
    expect_error("depth_limit", []
    {
        (void)json_read(chunks("[[1]]", 1), {100, 100, 1});
    });
    expect_error("invalid_argument", []
    {
        (void)json_read(chunks("1", 1), {0, 100, 1});
    });
    auto reader = json_new_reader(chunks("[\n\"中文\",?]", 1), {});
    (void)json_next(reader);
    try
    {
        (void)json_next(reader);
        expect(false);
    }
    catch (const runtime_failure& error)
    {
        expect(error.error().message.find("第 2 行第 6 列（字节偏移 11）") != std::string::npos);
    }
    auto owner = std::make_shared<int>(1);
    std::weak_ptr<int> weak = owner;
    auto held = json_new_reader([owner, done = false]() mutable
    {
        if (done)
        {
            return std::string{};
        }
        done = true;
        return std::string("[]");
    }, {});
    owner.reset();
    expect(!weak.expired());
    json_close(held);
    expect(weak.expired());
}

void check_writer_and_schema()
{
    std::string output;
    auto writer = json_new_writer([&](std::string_view part)
    {
        output += part;
    }, []
    {
    }, {12, 10, 128});
    json_write_value(writer, std::int64_t{1});
    expect_error("size_limit", [&]
    {
        json_write_value(writer, std::string(100, 'x'));
    });
    expect(output.starts_with("[1,"));
    expect_error("invalid_state", [&]
    {
        json_finish(writer);
    });
    json_close(writer);
    json_close(writer);
    const auto schema = json_parse_object("{\"type\":\"array\",\"minItems\":1,\"items\":{\"type\":\"integer\",\"minimum\":0}}");
    json_validate(json_parse("[0,1]"), schema);
    expect_error("schema_mismatch", [&]
    {
        json_validate(json_parse("[-1]"), schema);
    });
    expect_error("type_mismatch", [&]
    {
        json_validate(json_parse("[1.0]"), schema);
    });
    for (const auto text : {"{\"unknown\":true}", "{\"items\":2}",
        "{\"properties\":{\"absent\":{\"type\":\"bad\"}}}", "{\"minimum\":5,\"maximum\":1}"})
    {
        expect_error("invalid_schema", [&]
        {
            json_validate(std::any{}, json_parse_object(text));
        });
    }
    expect_error("missing_field", []
    {
        json_validate(tx_dict{}, json_parse_object("{\"required\":[\"id\"]}"));
    });
    auto cyclic = json_parse_object("{}");
    cyclic.emplace_back(std::string("self"), cyclic);
    expect_error("cyclic_value", [&]
    {
        json_validate(cyclic, {});
    });
    cyclic.clear();
}

void check_io_failures()
{
    auto reader = json_new_reader([first = true]() mutable
    {
        if (first)
        {
            first = false;
            return std::string("[1,");
        }
        throw runtime_failure({tx::error_kind::io, "operation_failed", "injected read failure"});
    }, {});
    (void)json_next(reader);
    expect_error("operation_failed", [&]
    {
        (void)json_next(reader);
    });
    expect_error("invalid_state", [&]
    {
        (void)json_next(reader);
    });
    auto owner = std::make_shared<int>(0);
    std::weak_ptr<int> weak = owner;
    auto writer = json_new_writer([owner](std::string_view)
    {
        if (++*owner > 1)
        {
            throw runtime_failure({tx::error_kind::io, "operation_failed", "injected write failure"});
        }
    }, []
    {
    }, {});
    owner.reset();
    expect_error("operation_failed", [&]
    {
        json_write_value(writer, std::int64_t{1});
    });
    expect_error("invalid_state", [&]
    {
        json_finish(writer);
    });
    json_close(writer);
    expect(weak.expired());
}

void check_large_array()
{
    int generated = -1;
    auto reader = json_new_reader([&]
    {
        if (generated++ == -1)
        {
            return std::string("[");
        }
        if (generated <= 20000)
        {
            return std::string(generated == 1 ? "" : ",") + std::to_string(generated);
        }
        return generated == 20001 ? std::string("]") : std::string{};
    }, {200000, 16, 128});
    expect(generated == 0);
    std::int64_t sum = 0;
    while (auto value = json_next(reader))
    {
        sum += std::any_cast<std::int64_t>(*value);
    }
    expect(sum == 200010000);
}

int main()
{
    check_chunks();
    check_limits_and_state();
    check_writer_and_schema();
    check_io_failures();
    check_large_array();
    std::cout << "JSON_STREAM_NATIVE_OK\n";
}
