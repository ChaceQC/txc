#include "stdlib/serde_direct.hpp"
#include "stdlib/json_scalar.hpp"
#include "stdlib/json_stream.hpp"

#include <charconv>

namespace tx_generated
{

serde_depth serde_depth::child(std::size_t logical_step, std::size_t wire_step) const
{
    return {logical + logical_step, wire + wire_step};
}

void serde_depth::check(bool encoding) const
{
    if (logical > 128 || wire > 128)
    {
        if (encoding)
        {
            serde_encode_error("depth_limit", "serde 值嵌套超过 128 层");
        }
        serde_decode_error("depth_limit", "serde 值嵌套超过 128 层");
    }
}

serde_writer::serde_writer(serde_format selected) : format(selected),
    output_({}, serde_byte_limit)
{
}

void serde_writer::integer(std::int64_t value)
{
    if (format == serde_format::cbor)
    {
        output_.append(value >= 0 ? cbor_head_bytes(0, value)
            : cbor_head_bytes(1, static_cast<std::uint64_t>(-(value + 1))));
        return;
    }
    char buffer[32];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    output_.append(std::string_view(buffer, result.ptr));
}

void serde_writer::floating(double value)
{
    if (format == serde_format::json)
    {
        json_detail::append_float(output_, value);
    }
    else
    {
        output_.append(cbor_float_bytes(value));
    }
}

void serde_writer::boolean(bool value)
{
    output_.append(format == serde_format::json ? (value ? "true" : "false")
        : (value ? "\xf5" : "\xf4"));
}

void serde_writer::text(std::string_view value)
{
    if (format == serde_format::json)
    {
        json_detail::append_string(output_, value);
        return;
    }
    if (!cbor_valid_utf8(value))
    {
        serde_encode_error("invalid_utf8", "CBOR 字符串包含无效 UTF-8");
    }
    output_.append(cbor_head_bytes(3, value.size()));
    output_.append(value);
}

void serde_writer::bytes(const byte_value& value)
{
    if (format == serde_format::json)
    {
        text(bytes_to_base64_url(value));
        return;
    }
    if (!value)
    {
        serde_encode_error("unsupported_type", "CBOR 字节值无效");
    }
    output_.append(cbor_head_bytes(2, value->size()));
    output_.append(std::string_view(reinterpret_cast<const char*>(value->data()), value->size()));
}

void serde_writer::null()
{
    output_.append(format == serde_format::json ? "null" : "\xf6");
}

void serde_writer::begin(bool object, std::size_t size, serde_depth depth)
{
    depth.check(true);
    if (size > 1000000 && (!object || format == serde_format::cbor))
    {
        serde_encode_error("size_limit", "serde 容器超过元素数上限");
    }
    if (format == serde_format::json)
    {
        output_.push_back(object ? '{' : '[');
    }
    else
    {
        output_.append(cbor_head_bytes(object ? 5 : 4, size));
    }
}

void serde_writer::end(bool object)
{
    if (format == serde_format::json)
    {
        output_.push_back(object ? '}' : ']');
    }
}

void serde_writer::separator(std::size_t index)
{
    if (format == serde_format::json && index != 0)
    {
        output_.push_back(',');
    }
}

void serde_writer::key(std::string_view name, std::int64_t number)
{
    if (format == serde_format::json)
    {
        text(name);
        output_.push_back(':');
    }
    else
    {
        integer(number);
    }
}

void serde_writer::dynamic(const std::any& value, std::size_t depth)
{
    // 仅 unknown=preserve 的真实未知内容进入通用格式编码器。
    const auto sink = [&](std::string_view chunk)
    {
        output_.append(chunk);
    };
    if (format == serde_format::json)
    {
        json_emit(value, sink, serde_byte_limit, 128, depth);
    }
    else
    {
        cbor_output output(sink, serde_cbor_limits);
        output.begin_value();
        cbor_emit_value(value, output, serde_cbor_limits, depth, active);
        output.end_value();
        output.flush();
    }
}

std::string serde_writer::finish()
{
    return output_.finish();
}

} // namespace tx_generated
