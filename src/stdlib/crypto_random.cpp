#include "stdlib/crypto_internal.hpp"

#include "stdlib/error.hpp"

#include <psa/crypto.h>

#include <algorithm>

namespace tx_generated::crypto
{

namespace
{

constexpr std::int64_t max_random_count = 1024 * 1024;
constexpr std::size_t random_chunk_size = 256;

} // namespace

[[noreturn]] void fail(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::runtime, code, message});
}

void require_success(int result, const char* message)
{
    if (result != 0)
    {
        fail("operation_failed", message);
    }
}

const unsigned char* byte_data(const byte_value& value) noexcept
{
    static const unsigned char empty = 0;
    return value->empty() ? &empty : value->data();
}

bool psa_ready()
{
    // 局部静态初始化保证并发下只执行一次，并保留首次初始化的失败状态。
    // 避免 ThinLTO 在符号解析阶段依赖 MinGW call_once 的外部模拟 TLS。
    static const psa_status_t status = psa_crypto_init();
    return status == PSA_SUCCESS;
}

void fill_random(std::span<std::uint8_t> output)
{
    if (output.empty())
    {
        return;
    }
    if (!psa_ready())
    {
        fail("random_failed", "初始化密码学随机源失败");
    }
    for (std::size_t offset = 0; offset < output.size();)
    {
        const auto count = std::min(random_chunk_size, output.size() - offset);
        if (psa_generate_random(output.data() + offset, count) != PSA_SUCCESS)
        {
            fail("random_failed", "生成密码学随机数失败");
        }
        offset += count;
    }
}

byte_value random_bytes(std::int64_t count)
{
    if (count < 0 || count > max_random_count)
    {
        fail("invalid_argument", "随机字节数必须在 0 到 1 MiB 之间");
    }
    sensitive_buffer result(static_cast<std::size_t>(count));
    fill_random(result.span());
    return result.release();
}

byte_value generate_key()
{
    return random_bytes(32);
}

} // namespace tx_generated::crypto
