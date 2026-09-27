#include "stdlib/crypto_internal.hpp"

#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>

#include <array>
#include <memory>

namespace tx_generated::crypto
{
namespace
{

template<class update>
void consume(const binary_stream& source, update&& append)
{
    std::array<char, 64 * 1024> block{};
    while (true)
    {
        const auto count = source->file.read_into(block.data(), block.size());
        if (count == 0)
        {
            return;
        }
        append(reinterpret_cast<const unsigned char*>(block.data()), count);
    }
}

} // namespace

byte_value sha256_stream(const binary_stream& source)
{
    mbedtls_sha256_context raw{};
    mbedtls_sha256_init(&raw);
    const std::unique_ptr<mbedtls_sha256_context, decltype(&mbedtls_sha256_free)>
        context(&raw, &mbedtls_sha256_free);
    require_success(mbedtls_sha256_starts(context.get(), 0),
                    "初始化流式 SHA-256 失败");
    consume(source, [&](const unsigned char* data, std::size_t size)
    {
        require_success(mbedtls_sha256_update(context.get(), data, size),
                        "更新流式 SHA-256 失败");
    });
    std::array<std::uint8_t, 32> digest{};
    require_success(mbedtls_sha256_finish(context.get(), digest.data()),
                    "完成流式 SHA-256 失败");
    return make_bytes({digest.begin(), digest.end()});
}

byte_value sha512_stream(const binary_stream& source)
{
    mbedtls_sha512_context raw{};
    mbedtls_sha512_init(&raw);
    const std::unique_ptr<mbedtls_sha512_context, decltype(&mbedtls_sha512_free)>
        context(&raw, &mbedtls_sha512_free);
    require_success(mbedtls_sha512_starts(context.get(), 0),
                    "初始化流式 SHA-512 失败");
    consume(source, [&](const unsigned char* data, std::size_t size)
    {
        require_success(mbedtls_sha512_update(context.get(), data, size),
                        "更新流式 SHA-512 失败");
    });
    std::array<std::uint8_t, 64> digest{};
    require_success(mbedtls_sha512_finish(context.get(), digest.data()),
                    "完成流式 SHA-512 失败");
    return make_bytes({digest.begin(), digest.end()});
}

byte_value hmac_sha256_stream(const secret::handle& key,
                              const binary_stream& source)
{
    const auto material = key->view();
    const auto* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!info)
    {
        fail("operation_failed", "HMAC-SHA256 不可用");
    }
    mbedtls_md_context_t raw{};
    mbedtls_md_init(&raw);
    const std::unique_ptr<mbedtls_md_context_t, decltype(&mbedtls_md_free)>
        context(&raw, &mbedtls_md_free);
    require_success(mbedtls_md_setup(context.get(), info, 1),
                    "初始化流式 HMAC-SHA256 失败");
    static constexpr unsigned char empty = 0;
    require_success(mbedtls_md_hmac_starts(context.get(),
        material.empty() ? &empty : material.data(), material.size()),
        "初始化流式 HMAC-SHA256 失败");
    consume(source, [&](const unsigned char* data, std::size_t size)
    {
        require_success(mbedtls_md_hmac_update(context.get(), data, size),
                        "更新流式 HMAC-SHA256 失败");
    });
    sensitive_buffer digest(32);
    require_success(mbedtls_md_hmac_finish(context.get(), digest.data()),
                    "完成流式 HMAC-SHA256 失败");
    return digest.release();
}

} // namespace tx_generated::crypto
