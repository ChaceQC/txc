#include "stdlib/cbor_internal.hpp"
#include "stdlib/error.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace tx_generated
{
namespace
{

[[noreturn]] void encode_error(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::runtime, code, message});
}

void emit_integer(std::int64_t value, cbor_output& output)
{
    output.append(value >= 0 ? cbor_head_bytes(0, static_cast<std::uint64_t>(value))
        : cbor_head_bytes(1, static_cast<std::uint64_t>(-(value + 1))));
}

void emit_text(std::string_view value, cbor_output& output)
{
    if (!cbor_valid_utf8(value))
    {
        encode_error("invalid_utf8", "CBOR 字符串包含无效 UTF-8");
    }
    output.append(cbor_head_bytes(3, value.size()));
    output.append(value);
}

void emit_bytes(const byte_value& value, cbor_output& output)
{
    if (!value)
    {
        encode_error("unsupported_type", "CBOR 字节值无效");
    }
    output.append(cbor_head_bytes(2, value->size()));
    output.append(std::string_view(reinterpret_cast<const char*>(value->data()),
                                   value->size()));
}

void emit_array(const tx_array& value, cbor_output& output,
                const cbor_limits& limits, std::size_t depth,
                std::unordered_set<const void*>& active)
{
    if (value.size() > static_cast<std::size_t>(limits.max_items))
    {
        encode_error("size_limit", "CBOR 数组长度超过元素上限");
    }
    if (!active.insert(value.identity()).second)
    {
        encode_error("cyclic_value", "CBOR 不能编码循环数组");
    }
    output.append(cbor_head_bytes(4, value.size()));
    for (const auto& item : value)
    {
        cbor_emit_value(item, output, limits, depth + 1, active);
    }
    active.erase(value.identity());
}

void emit_map(const tx_dict& value, cbor_output& output,
              const cbor_limits& limits, std::size_t depth,
              std::unordered_set<const void*>& active)
{
    if (value.size() > static_cast<std::size_t>(limits.max_items))
    {
        encode_error("size_limit", "CBOR 映射长度超过元素上限");
    }
    if (!active.insert(value.identity()).second)
    {
        encode_error("cyclic_value", "CBOR 不能编码循环映射");
    }
    struct entry
    {
        std::string key;
        const std::any* value;
    };
    std::vector<entry> entries;
    entries.reserve(value.size());
    value.for_each([&](const std::any& key, const std::any& item)
    {
        entries.push_back({cbor_encode_key(key), &item});
    });
    std::sort(entries.begin(), entries.end(), [](const entry& left, const entry& right)
    {
        return cbor_key_less(left.key, right.key);
    });
    output.append(cbor_head_bytes(5, entries.size()));
    std::string previous;
    for (const auto& item : entries)
    {
        if (!previous.empty() && previous == item.key)
        {
            encode_error("duplicate_key", "CBOR 映射键的编码重复");
        }
        previous = item.key;
        output.append(item.key);
        cbor_emit_value(*item.value, output, limits, depth + 1, active);
    }
    active.erase(value.identity());
}

} // namespace

std::string cbor_encode_key(const std::any& key)
{
    std::string result;
    const cbor_limits limits{67108864, 67108864, 128, 1000000};
    cbor_output output([&](std::string_view chunk)
    {
        result.append(chunk);
    }, limits);
    output.begin_value();
    if (!key.has_value())
    {
        output.append(std::string_view("\xf6", 1));
    }
    else if (const auto* number = std::any_cast<std::int64_t>(&key))
    {
        emit_integer(*number, output);
    }
    else if (const auto* number = std::any_cast<double>(&key))
    {
        if (!std::isfinite(*number))
        {
            encode_error("unsupported_type", "CBOR 映射键不能是 NaN 或无穷大");
        }
        output.append(cbor_float_bytes(*number));
    }
    else if (const auto* flag = std::any_cast<bool>(&key))
    {
        output.append(std::string_view(*flag ? "\xf5" : "\xf4", 1));
    }
    else if (const auto* text = std::any_cast<std::string>(&key))
    {
        emit_text(*text, output);
    }
    else
    {
        encode_error("unsupported_type", "CBOR 映射键不能映射到 TX dict");
    }
    output.end_value();
    output.flush();
    return result;
}

void cbor_emit_value(const std::any& value, cbor_output& output,
                     const cbor_limits& limits, std::size_t depth,
                     std::unordered_set<const void*>& active)
{
    if (depth > static_cast<std::size_t>(limits.max_depth))
    {
        encode_error("depth_limit", "CBOR 嵌套超过深度上限");
    }
    if (!value.has_value())
    {
        output.append(std::string_view("\xf6", 1));
    }
    else if (const auto* flag = std::any_cast<bool>(&value))
    {
        output.append(std::string_view(*flag ? "\xf5" : "\xf4", 1));
    }
    else if (const auto* number = std::any_cast<std::int64_t>(&value))
    {
        emit_integer(*number, output);
    }
    else if (const auto* number = std::any_cast<double>(&value))
    {
        output.append(cbor_float_bytes(*number));
    }
    else if (const auto* text = std::any_cast<std::string>(&value))
    {
        emit_text(*text, output);
    }
    else if (const auto* bytes = std::any_cast<byte_value>(&value))
    {
        emit_bytes(*bytes, output);
    }
    else if (const auto* array = std::any_cast<tx_array>(&value))
    {
        emit_array(*array, output, limits, depth, active);
    }
    else if (const auto* map = std::any_cast<tx_dict>(&value))
    {
        emit_map(*map, output, limits, depth, active);
    }
    else
    {
        encode_error("unsupported_type", "CBOR 只支持 int、float、str、bytes、array、dict、bool 和 none");
    }
}

byte_value cbor_encode(const std::any& value, const cbor_limits& limits)
{
    cbor_check_limits(limits);
    std::string result;
    cbor_output output([&](std::string_view chunk)
    {
        result.append(chunk);
    }, limits);
    output.begin_value();
    std::unordered_set<const void*> active;
    cbor_emit_value(value, output, limits, 0, active);
    output.end_value();
    output.flush();
    return make_bytes({result.begin(), result.end()});
}

void cbor_write(format_sink sink, const std::any& value, const cbor_limits& limits)
{
    cbor_check_limits(limits);
    cbor_output output(std::move(sink), limits);
    output.begin_value();
    std::unordered_set<const void*> active;
    cbor_emit_value(value, output, limits, 0, active);
    output.end_value();
    output.flush();
}

} // namespace tx_generated
