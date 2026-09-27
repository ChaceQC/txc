#include "stdlib/cbor_internal.hpp"
#include "stdlib/error.hpp"

#include <utility>

namespace tx_generated
{

struct cbor_reader_state
{
    std::unique_ptr<format_input> input;
    std::function<void()> guard;
    cbor_limits limits;
    std::uint64_t remaining = 0;
    bool finished = false;
    bool failed = false;
};

struct cbor_writer_state
{
    std::unique_ptr<cbor_output> output;
    std::function<void()> flush;
    cbor_limits limits;
    std::uint64_t expected = 0;
    std::uint64_t written = 0;
    bool finished = false;
    bool failed = false;
};

namespace
{

[[noreturn]] void invalid_state()
{
    throw runtime_failure({tx::error_kind::runtime, "invalid_state",
        "CBOR 游标已关闭、结束或因先前错误失效"});
}

} // namespace

std::any cbor_decode(const byte_value& data, const cbor_limits& limits)
{
    cbor_check_limits(limits);
    if (!data)
    {
        invalid_state();
    }
    if (data->size() > static_cast<std::uint64_t>(limits.max_bytes) ||
        data->size() > static_cast<std::uint64_t>(limits.max_value_bytes))
    {
        throw runtime_failure({tx::error_kind::parse, "size_limit",
            "CBOR 输入超过配置的字节上限（字节偏移 0）"});
    }
    format_input input(std::string_view(
        reinterpret_cast<const char*>(data->data()), data->size()), "CBOR");
    if (input.peek() < 0)
    {
        input.fail("empty_input", "CBOR 输入不能为空");
    }
    input.begin_value(limits.max_value_bytes);
    auto result = cbor_parse_value(input, limits, 0);
    input.end_value();
    cbor_require_end(input);
    return result;
}

std::any cbor_read(format_source source, const cbor_limits& limits)
{
    cbor_check_limits(limits);
    format_input input(std::move(source), "CBOR", limits.max_bytes);
    if (input.peek() < 0)
    {
        input.fail("empty_input", "CBOR 输入不能为空");
    }
    input.begin_value(limits.max_value_bytes);
    auto result = cbor_parse_value(input, limits, 0);
    input.end_value();
    cbor_require_end(input);
    return result;
}

cbor_reader cbor_new_reader(format_source source, const cbor_limits& limits,
                            std::function<void()> guard)
{
    cbor_check_limits(limits);
    auto result = std::make_shared<cbor_reader_state>();
    result->limits = limits;
    result->guard = std::move(guard);
    result->input = std::make_unique<format_input>(
        std::move(source), "CBOR", limits.max_bytes);
    if (result->input->peek() < 0)
    {
        result->input->fail("empty_input", "CBOR 输入不能为空");
    }
    const auto head = cbor_read_head(*result->input);
    if (head.major != 4)
    {
        result->input->fail("type_mismatch", "CBOR 增量 reader 需要根数组");
    }
    if (head.argument > static_cast<std::uint64_t>(limits.max_items))
    {
        result->input->fail("size_limit", "CBOR 根数组超过元素上限");
    }
    result->remaining = head.argument;
    return result;
}

std::optional<std::any> cbor_next(const cbor_reader& reader)
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
        if (reader->remaining == 0)
        {
            cbor_require_end(input);
            reader->finished = true;
            return std::nullopt;
        }
        input.begin_value(reader->limits.max_value_bytes);
        auto result = cbor_parse_value(input, reader->limits, 1);
        input.end_value();
        --reader->remaining;
        return result;
    }
    catch (...)
    {
        reader->failed = true;
        throw;
    }
}

cbor_writer cbor_new_writer(format_sink sink, std::function<void()> flush,
                            std::int64_t count, const cbor_limits& limits)
{
    cbor_check_limits(limits);
    if (count < 0 || count > limits.max_items)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_argument",
            "CBOR 根数组元素数量无效"});
    }
    auto result = std::make_shared<cbor_writer_state>();
    result->limits = limits;
    result->flush = std::move(flush);
    result->expected = static_cast<std::uint64_t>(count);
    result->output = std::make_unique<cbor_output>(std::move(sink), limits);
    result->output->append(cbor_head_bytes(4, result->expected));
    return result;
}

void cbor_write_value(const cbor_writer& writer, const std::any& value)
{
    if (!writer || !writer->output || writer->failed || writer->finished ||
        writer->written >= writer->expected)
    {
        invalid_state();
    }
    try
    {
        writer->output->begin_value();
        std::unordered_set<const void*> active;
        cbor_emit_value(value, *writer->output, writer->limits, 1, active);
        writer->output->end_value();
        ++writer->written;
    }
    catch (...)
    {
        writer->failed = true;
        throw;
    }
}

void cbor_finish(const cbor_writer& writer)
{
    if (!writer || !writer->output || writer->failed)
    {
        invalid_state();
    }
    if (writer->finished)
    {
        return;
    }
    try
    {
        if (writer->written != writer->expected)
        {
            throw runtime_failure({tx::error_kind::runtime, "invalid_state",
                "CBOR 根数组的写入数量与声明数量不符"});
        }
        writer->output->flush();
        writer->flush();
        writer->finished = true;
    }
    catch (...)
    {
        writer->failed = true;
        throw;
    }
}

void cbor_close(const cbor_reader& reader)
{
    if (reader)
    {
        reader->input.reset();
        reader->guard = {};
    }
}

void cbor_close(const cbor_writer& writer)
{
    if (writer)
    {
        writer->output.reset();
        writer->flush = {};
    }
}

} // namespace tx_generated
