#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/crypto.hpp"

#include <any>
#include <cstdint>
#include <utility>

namespace
{

const tx_generated::byte_value& byte_argument(const void* value)
{
    return tx_generated::bytes_of(*static_cast<const std::any*>(value));
}

template<class operation>
int bytes_result(void** result, operation&& run) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(
            std::forward<operation>(run)());
    });
}

} // namespace

extern "C" int txrt_crypto_random_bytes(std::int64_t count,
                                           void** result) noexcept
{
    return bytes_result(result, [&]
    {
        return tx_generated::crypto::random_bytes(count);
    });
}

extern "C" int txrt_crypto_generate_key(void** result) noexcept
{
    return bytes_result(result, []
    {
        return tx_generated::crypto::generate_key();
    });
}

extern "C" int txrt_crypto_sha256(const void* data, void** result) noexcept
{
    return bytes_result(result, [&]
    {
        return tx_generated::crypto::sha256(byte_argument(data));
    });
}

extern "C" int txrt_crypto_sha512(const void* data, void** result) noexcept
{
    return bytes_result(result, [&]
    {
        return tx_generated::crypto::sha512(byte_argument(data));
    });
}

extern "C" int txrt_crypto_hmac_sha256(const void* key, const void* data,
                                         void** result) noexcept
{
    return bytes_result(result, [&]
    {
        return tx_generated::crypto::hmac_sha256(byte_argument(key),
                                                   byte_argument(data));
    });
}

extern "C" int txrt_crypto_secure_equal(const void* left, const void* right,
                                          bool* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::crypto::secure_equal(byte_argument(left),
                                                       byte_argument(right));
    });
}

extern "C" int txrt_crypto_hkdf_sha256(const void* ikm, const void* salt,
                                         const void* info, std::int64_t length,
                                         void** result) noexcept
{
    return bytes_result(result, [&]
    {
        return tx_generated::crypto::hkdf_sha256(byte_argument(ikm),
            byte_argument(salt), byte_argument(info), length);
    });
}

extern "C" int txrt_crypto_pbkdf2_sha256(const void* password,
                                           const void* salt,
                                           std::int64_t iterations,
                                           std::int64_t length,
                                           void** result) noexcept
{
    return bytes_result(result, [&]
    {
        return tx_generated::crypto::pbkdf2_sha256(byte_argument(password),
            byte_argument(salt), iterations, length);
    });
}

extern "C" int txrt_crypto_encrypt(const void* key, const void* plaintext,
                                     const void* aad, void** result) noexcept
{
    return bytes_result(result, [&]
    {
        return tx_generated::crypto::encrypt(byte_argument(key),
            byte_argument(plaintext), byte_argument(aad));
    });
}

extern "C" int txrt_crypto_decrypt(const void* key, const void* encrypted,
                                     const void* aad, void** result) noexcept
{
    return bytes_result(result, [&]
    {
        return tx_generated::crypto::decrypt(byte_argument(key),
            byte_argument(encrypted), byte_argument(aad));
    });
}
