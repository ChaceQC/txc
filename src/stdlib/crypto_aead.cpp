#include "stdlib/crypto_internal.hpp"

#include <mbedtls/gcm.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace tx_generated::crypto
{

namespace
{

constexpr std::size_t nonce_size = 12;
constexpr std::size_t tag_size = 16;
constexpr std::size_t header_size = 18;
constexpr std::size_t overhead = header_size + tag_size;
constexpr std::array<std::uint8_t, 6> prefix{'T', 'X', 'C', 'G', 1, 1};

class gcm_context
{
public:
    gcm_context()
    {
        mbedtls_gcm_init(&value_);
    }

    ~gcm_context()
    {
        mbedtls_gcm_free(&value_);
    }

    gcm_context(const gcm_context&) = delete;
    gcm_context& operator=(const gcm_context&) = delete;

    [[nodiscard]] mbedtls_gcm_context* get() noexcept
    {
        return &value_;
    }

private:
    mbedtls_gcm_context value_{};
};

void require_key(const byte_value& key)
{
    if (key->size() != 32)
    {
        fail("invalid_argument", "AES-256-GCM 密钥必须恰好为 32 字节");
    }
}

void set_key(gcm_context& context, const byte_value& key)
{
    require_success(mbedtls_gcm_setkey(context.get(), MBEDTLS_CIPHER_ID_AES,
        byte_data(key), 256), "设置 AES-256-GCM 密钥失败");
}

std::vector<std::uint8_t> authentication_data(
    const std::uint8_t* header, const byte_value& aad)
{
    if (aad->size() > std::numeric_limits<std::size_t>::max() - header_size)
    {
        fail("size_limit", "附加认证数据过长");
    }
    std::vector<std::uint8_t> result;
    result.reserve(header_size + aad->size());
    result.insert(result.end(), header, header + header_size);
    result.insert(result.end(), aad->begin(), aad->end());
    return result;
}

} // namespace

byte_value encrypt(const byte_value& key, const byte_value& plaintext,
                   const byte_value& aad)
{
    require_key(key);
    if (plaintext->size() > static_cast<std::size_t>(
            std::numeric_limits<std::int64_t>::max()) - overhead)
    {
        fail("size_limit", "加密结果超出 bytes 长度范围");
    }

    sensitive_buffer result(plaintext->size() + overhead);
    std::copy(prefix.begin(), prefix.end(), result.data());
    fill_random(std::span<std::uint8_t>(result.data() + prefix.size(),
                                        nonce_size));
    const auto auth = authentication_data(result.data(), aad);
    gcm_context context;
    set_key(context, key);
    require_success(mbedtls_gcm_crypt_and_tag(context.get(),
        MBEDTLS_GCM_ENCRYPT, plaintext->size(), result.data() + prefix.size(),
        nonce_size, auth.data(), auth.size(), byte_data(plaintext),
        result.data() + header_size, tag_size,
        result.data() + header_size + plaintext->size()),
        "AES-256-GCM 加密失败");
    return result.release();
}

byte_value decrypt(const byte_value& key, const byte_value& encrypted,
                   const byte_value& aad)
{
    require_key(key);
    if (encrypted->size() < overhead ||
        !std::equal(prefix.begin(), prefix.end(), encrypted->begin()))
    {
        fail("invalid_format", "加密数据格式或版本无效");
    }

    const auto plaintext_size = encrypted->size() - overhead;
    const auto auth = authentication_data(encrypted->data(), aad);
    sensitive_buffer plaintext(plaintext_size);
    std::uint8_t empty_output = 0;
    gcm_context context;
    set_key(context, key);
    const int status = mbedtls_gcm_auth_decrypt(context.get(), plaintext_size,
        encrypted->data() + prefix.size(), nonce_size, auth.data(), auth.size(),
        encrypted->data() + header_size + plaintext_size, tag_size,
        encrypted->data() + header_size,
        plaintext_size == 0 ? &empty_output : plaintext.data());
    if (status == MBEDTLS_ERR_GCM_AUTH_FAILED)
    {
        fail("authentication_failed", "加密数据认证失败");
    }
    require_success(status, "AES-256-GCM 解密失败");
    return plaintext.release();
}

} // namespace tx_generated::crypto
