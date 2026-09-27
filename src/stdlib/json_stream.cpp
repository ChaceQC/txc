#include "stdlib/json_stream.hpp"
#include "stdlib/json_parser.hpp"

#include <algorithm>
#include <utility>

namespace tx_generated
{

struct json_reader_state
{
    std::unique_ptr<format_input> input;
    std::function<void()> guard;
    json_limits limits;
    bool first = true;
    bool finished = false;
    bool failed = false;
};

struct json_writer_state
{
    format_sink sink;
    std::function<void()> flush;
    json_limits limits;
    std::uint64_t written = 0;
    bool first = true;
    bool finished = false;
    bool failed = false;

    void append(std::string_view text)
    {
        if (text.size() > static_cast<std::uint64_t>(limits.max_bytes) - written)
        {
            throw runtime_failure({tx::error_kind::runtime, "size_limit",
                "JSON 输出超过总字节上限"});
        }
        sink(text);
        written += text.size();
    }
};

namespace
{

[[noreturn]] void invalid_state()
{
    throw runtime_failure({tx::error_kind::runtime, "invalid_state",
        "JSON 游标已关闭、结束或因先前错误失效"});
}

} // namespace

void json_check_limits(const json_limits& limits)
{
    if (limits.max_bytes <= 0 || limits.max_bytes > 1099511627776LL ||
        limits.max_value_bytes <= 0 || limits.max_value_bytes > 67108864 ||
        limits.max_depth <= 0 || limits.max_depth > 128)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_argument",
            "JSON 总字节、单值字节或深度限额无效"});
    }
}

std::any json_read(format_source source, const json_limits& limits)
{
    json_check_limits(limits);
    format_input input(std::move(source), "JSON", limits.max_bytes);
    json_parser parser(input, limits.max_depth);
    parser.skip_space();
    if (input.peek() < 0)
    {
        input.fail("empty_input", "JSON 文本不能为空");
    }
    input.begin_value(limits.max_value_bytes);
    auto result = parser.parse_value(0);
    input.end_value();
    parser.require_end();
    return result;
}

json_reader json_new_reader(format_source source, const json_limits& limits,
                            std::function<void()> guard)
{
    json_check_limits(limits);
    auto result = std::make_shared<json_reader_state>();
    result->limits = limits;
    result->guard = std::move(guard);
    result->input = std::make_unique<format_input>(std::move(source), "JSON", limits.max_bytes);
    json_parser parser(*result->input, limits.max_depth);
    parser.skip_space();
    if (!result->input->take('['))
    {
        result->input->fail("type_mismatch", "JSON 增量 reader 需要根数组");
    }
    return result;
}

std::optional<std::any> json_next(const json_reader& reader)
{
    if (!reader || !reader->input || reader->failed)
    {
        invalid_state();
    }
    try
    {
        if (reader->guard)
        {
            reader->guard();
        }
        if (reader->finished)
        {
            return std::nullopt;
        }
        auto& input = *reader->input;
        json_parser parser(input, reader->limits.max_depth);
        parser.skip_space();
        if (input.take(']'))
        {
            parser.require_end();
            reader->finished = true;
            return std::nullopt;
        }
        if (!reader->first)
        {
            if (!input.take(','))
            {
                input.fail("invalid_syntax", "数组元素后需要逗号或右方括号");
            }
            parser.skip_space();
        }
        input.begin_value(reader->limits.max_value_bytes);
        auto result = parser.parse_value(1);
        input.end_value();
        reader->first = false;
        return result;
    }
    catch (...)
    {
        reader->failed = true;
        throw;
    }
}

void json_close(const json_reader& reader)
{
    if (reader)
    {
        reader->input.reset();
        reader->guard = {};
    }
}

void json_write(format_sink sink, const std::any& value, const json_limits& limits)
{
    json_check_limits(limits);
    json_emit(value, std::move(sink), std::min(limits.max_bytes, limits.max_value_bytes),
              limits.max_depth);
}

json_writer json_new_writer(format_sink sink, std::function<void()> flush,
                            const json_limits& limits)
{
    json_check_limits(limits);
    auto result = std::make_shared<json_writer_state>();
    result->sink = std::move(sink);
    result->flush = std::move(flush);
    result->limits = limits;
    result->append("[");
    return result;
}

void json_write_value(const json_writer& writer, const std::any& value)
{
    if (!writer || !writer->sink || writer->failed || writer->finished)
    {
        invalid_state();
    }
    try
    {
        if (!writer->first)
        {
            writer->append(",");
        }
        json_emit(value, [&](std::string_view text)
        {
            writer->append(text);
        }, writer->limits.max_value_bytes, writer->limits.max_depth, 1);
        writer->first = false;
    }
    catch (...)
    {
        writer->failed = true;
        throw;
    }
}

void json_finish(const json_writer& writer)
{
    if (!writer || !writer->sink || writer->failed)
    {
        invalid_state();
    }
    if (writer->finished)
    {
        return;
    }
    try
    {
        writer->append("]");
        writer->flush();
        writer->finished = true;
    }
    catch (...)
    {
        writer->failed = true;
        throw;
    }
}

void json_close(const json_writer& writer)
{
    if (writer)
    {
        writer->sink = {};
        writer->flush = {};
    }
}

} // namespace tx_generated
