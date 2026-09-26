#include "stdlib/crypto_internal.hpp"

#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>

// 该内部头文件没有自行声明 C++ 链接属性，需匹配静态库中的 C 符号。
extern "C"
{
#include <mbedtls/constant_time.h>
}

#include <array>
#include <cstdint>

namespace tx_generated::crypto
{

byte_value sha256(const byte_value& data)
{
    std::array<std::uint8_t, 32> digest{};
    require_success(mbedtls_sha256(byte_data(data), data->size(),
                                   digest.data(), 0), "计算 SHA-256 失败");
    return make_bytes({digest.begin(), digest.end()});
}

byte_value sha512(const byte_value& data)
{
    std::array<std::uint8_t, 64> digest{};
    require_success(mbedtls_sha512(byte_data(data), data->size(),
                                   digest.data(), 0), "计算 SHA-512 失败");
    return make_bytes({digest.begin(), digest.end()});
}

byte_value hmac_sha256(const byte_value& key, const byte_value& data)
{
    sensitive_buffer mac(32);
    const auto* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!info)
    {
        fail("operation_failed", "HMAC-SHA256 不可用");
    }
    require_success(mbedtls_md_hmac(info, byte_data(key), key->size(),
        byte_data(data), data->size(), mac.data()), "计算 HMAC-SHA256 失败");
    return mac.release();
}

bool secure_equal(const byte_value& left, const byte_value& right)
{
    if (left->size() != right->size())
    {
        return false;
    }
    return mbedtls_ct_memcmp(byte_data(left), byte_data(right),
                             left->size()) == 0;
}

} // namespace tx_generated::crypto
