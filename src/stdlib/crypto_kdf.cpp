#include "stdlib/crypto_internal.hpp"

#include <mbedtls/hkdf.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>

#include <cstdint>

namespace tx_generated::crypto
{

namespace
{

const mbedtls_md_info_t* sha256_info()
{
    const auto* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!info)
    {
        fail("operation_failed", "SHA-256 密钥派生不可用");
    }
    return info;
}

} // namespace

byte_value hkdf_sha256(const byte_value& ikm, const byte_value& salt,
                       const byte_value& info, std::int64_t length)
{
    if (length < 1 || length > 255 * 32)
    {
        fail("invalid_argument", "HKDF 输出长度必须在 1 到 8160 之间");
    }
    sensitive_buffer result(static_cast<std::size_t>(length));
    require_success(mbedtls_hkdf(sha256_info(), byte_data(salt), salt->size(),
        byte_data(ikm), ikm->size(), byte_data(info), info->size(),
        result.data(), result.size()), "计算 HKDF-SHA256 失败");
    return result.release();
}

byte_value pbkdf2_sha256(const byte_value& password, const byte_value& salt,
                         std::int64_t iterations, std::int64_t length)
{
    if (iterations < 1 || iterations > 10'000'000 ||
        length < 1 || length > 1024)
    {
        fail("invalid_argument", "PBKDF2 迭代次数或输出长度无效");
    }
    sensitive_buffer result(static_cast<std::size_t>(length));
    require_success(mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,
        byte_data(password), password->size(), byte_data(salt), salt->size(),
        static_cast<unsigned int>(iterations),
        static_cast<std::uint32_t>(length), result.data()),
        "计算 PBKDF2-HMAC-SHA256 失败");
    return result.release();
}

} // namespace tx_generated::crypto
