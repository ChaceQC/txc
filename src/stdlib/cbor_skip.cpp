#include "stdlib/cbor_internal.hpp"

namespace tx_generated
{

void cbor_skip_value(format_input& input, const cbor_limits& limits, std::size_t depth)
{
    if (depth > static_cast<std::size_t>(limits.max_depth))
    {
        input.fail("depth_limit", "CBOR 嵌套超过深度上限");
    }
    const int major = input.peek() < 0 ? -1 : input.peek() >> 5;
    if (major != 4 && major != 5)
    {
        (void)cbor_parse_value(input, limits, depth);
        return;
    }
    const auto head = cbor_read_head(input);
    if (head.argument > static_cast<std::uint64_t>(limits.max_items))
    {
        input.fail("size_limit", "CBOR 容器超过元素数上限");
    }
    std::string previous;
    bool float_zero_seen = false;
    for (std::uint64_t index = 0; index < head.argument; ++index)
    {
        if (major == 5)
        {
            const auto key = cbor_parse_value(input, limits, depth + 1);
            // dict 将 +0.0/-0.0 视为相同键，规范字节序本身不能排除此重复。
            if (const auto* number = std::any_cast<double>(&key); number && *number == 0)
            {
                if (float_zero_seen)
                {
                    input.fail("duplicate_key", "CBOR 映射包含重复键");
                }
                float_zero_seen = true;
            }
            // 复用键编码器的类型限制；非规范/重复键由入口的旧诊断路径精确裁定。
            const auto encoded = cbor_encode_key(key);
            if (!previous.empty() && !cbor_key_less(previous, encoded))
            {
                input.fail(previous == encoded ? "duplicate_key" : "non_canonical",
                    "CBOR 映射键重复或未按规范顺序排列");
            }
            previous = encoded;
        }
        cbor_skip_value(input, limits, depth + 1);
    }
}

} // namespace tx_generated
