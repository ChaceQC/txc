#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/crypto.hpp"

#include <any>
#include <string>

namespace
{

const tx_generated::binary_stream& binary_argument(const void* value)
{
    const auto& stream = std::any_cast<const tx_generated::binary_stream&>(
        *static_cast<const std::any*>(value));
    if (!stream)
    {
        throw tx_generated::runtime_failure({tx::error_kind::io,
            "closed_stream", "二进制文件流未打开或已关闭"});
    }
    return stream;
}

const tx_generated::secret::handle& secret_argument(const void* value)
{
    return std::any_cast<const tx_generated::secret::handle&>(
        *static_cast<const std::any*>(value));
}

const tx_generated::byte_value& byte_argument(const void* value)
{
    return tx_generated::bytes_of(*static_cast<const std::any*>(value));
}

const std::string& text_argument(const void* value)
{
    return *static_cast<const std::string*>(value);
}

template<class operation>
int bytes_result(void** result, operation&& run) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(run());
    });
}

} // namespace

extern "C" int txrt_crypto_sha256_stream(const void* source,
                                         void** result) noexcept
{
    return bytes_result(result, [&]
    {
        return tx_generated::crypto::sha256_stream(binary_argument(source));
    });
}

extern "C" int txrt_crypto_sha512_stream(const void* source,
                                         void** result) noexcept
{
    return bytes_result(result, [&]
    {
        return tx_generated::crypto::sha512_stream(binary_argument(source));
    });
}

extern "C" int txrt_crypto_hmac_sha256_stream(const void* key,
    const void* source, void** result) noexcept
{
    return bytes_result(result, [&]
    {
        return tx_generated::crypto::hmac_sha256_stream(
            secret_argument(key), binary_argument(source));
    });
}

extern "C" int txrt_crypto_encrypt_file(const void* key,
    const void* source_path, const void* target_path, const void* aad,
    const void* key_id) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        tx_generated::crypto::encrypt_file(secret_argument(key),
            text_argument(source_path), text_argument(target_path),
            byte_argument(aad), text_argument(key_id));
    });
}

extern "C" int txrt_crypto_decrypt_file(const void* key,
    const void* source_path, const void* target_path, const void* aad,
    const void* expected_key_id) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        tx_generated::crypto::decrypt_file(secret_argument(key),
            text_argument(source_path), text_argument(target_path),
            byte_argument(aad), text_argument(expected_key_id));
    });
}

extern "C" int txrt_crypto_file_key_id(const void* path,
                                       void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::crypto::file_key_id(text_argument(path)));
    });
}
