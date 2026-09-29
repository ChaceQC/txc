#include "stdlib/serde_direct.hpp"

namespace tx_generated
{

serde_reader::serde_reader(serde_format selected, std::string_view input)
    : format(selected), input_(input, selected == serde_format::json ? "JSON" : "CBOR"),
      json_(input_, 128, true)
{
    if (input.size() > serde_byte_limit)
    {
        serde_decode_error("size_limit", "serde 输入超过 16 MiB");
    }
    if (format == serde_format::json)
    {
        json_.skip_space();
    }
    if (input_.peek() < 0)
    {
        input_.fail("empty_input", "输入不能为空");
    }
}

serde_sequence serde_reader::begin(bool object, serde_depth depth)
{
    depth.check(false);
    serde_sequence result;
    result.object = object;
    if (format == serde_format::json)
    {
        json_.skip_space();
        if (!input_.take(object ? '{' : '['))
        {
            serde_decode_error("type_mismatch", "serde 字段需要映射或数组");
        }
    }
    else
    {
        const auto head = cbor_read_head(input_);
        if (head.major != (object ? 5 : 4))
        {
            serde_decode_error("type_mismatch", "serde 字段需要映射或数组");
        }
        result.size = head.argument;
        if (result.size > 1000000)
        {
            input_.fail("size_limit", "CBOR 容器超过元素数上限");
        }
    }
    return result;
}

bool serde_reader::next(serde_sequence& sequence)
{
    if (format == serde_format::cbor)
    {
        return sequence.index++ < sequence.size;
    }
    json_.skip_space();
    if (input_.take(sequence.object ? '}' : ']'))
    {
        return false;
    }
    if (sequence.index != 0)
    {
        if (!input_.take(','))
        {
            input_.fail("invalid_syntax", "元素后需要逗号或容器结束符");
        }
        json_.skip_space();
    }
    if (++sequence.index > 1000000)
    {
        input_.fail("size_limit", "serde 容器超过元素数上限");
    }
    return true;
}

serde_key serde_reader::key(serde_sequence& sequence, std::size_t depth)
{
    if (format == serde_format::json)
    {
        if (input_.peek() != '"')
        {
            input_.fail("invalid_syntax", "对象字段名必须是字符串");
        }
        auto result = json_.parse_string();
        json_.skip_space();
        if (!input_.take(':'))
        {
            input_.fail("invalid_syntax", "对象字段名后需要冒号");
        }
        json_.skip_space();
        return {std::move(result), 0};
    }
    auto result = cbor_parse_value(input_, serde_cbor_limits, depth);
    const auto* number = std::any_cast<std::int64_t>(&result);
    if (!number || *number < 0)
    {
        serde_decode_error("type_mismatch", "serde CBOR 字段编号必须是正整数");
    }
    const auto encoded = cbor_head_bytes(0, *number);
    if (!sequence.previous_key.empty() && !cbor_key_less(sequence.previous_key, encoded))
    {
        input_.fail(sequence.previous_key == encoded ? "duplicate_key" : "non_canonical",
            "CBOR 映射键重复或未按规范顺序排列");
    }
    sequence.previous_key = encoded;
    return {{}, *number};
}

std::any serde_reader::scalar(std::size_t depth)
{
    if (format == serde_format::json)
    {
        json_.skip_space();
        return json_.parse_value(depth);
    }
    return cbor_parse_value(input_, serde_cbor_limits, depth);
}

bool serde_reader::take_null()
{
    if (format == serde_format::json)
    {
        json_.skip_space();
        if (input_.peek() != 'n')
        {
            return false;
        }
        (void)json_.parse_value(0);
        return true;
    }
    return input_.take('\xf6');
}

void serde_reader::skip(std::size_t depth)
{
    if (format == serde_format::json)
    {
        json_.skip_space();
        json_.skip_value(depth);
    }
    else
    {
        cbor_skip_value(input_, serde_cbor_limits, depth);
    }
}

void serde_reader::finish()
{
    if (format == serde_format::json)
    {
        json_.require_end();
    }
    else
    {
        cbor_require_end(input_);
    }
}

} // namespace tx_generated
