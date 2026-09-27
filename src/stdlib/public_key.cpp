#include "stdlib/public_key.hpp"

#include "stdlib/crypto_internal.hpp"
#include "stdlib/error.hpp"

#include <mbedtls/hkdf.h>
#include <mbedtls/md.h>
#include <sodium.h>

#include <array>
#include <cstdint>
#include <mutex>
#include <span>
#include <vector>

namespace tx_generated::public_key
{
namespace
{

[[noreturn]] void fail(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::security, code, message});
}

void ready()
{
    static std::once_flag once;
    static int status = -1;
    std::call_once(once, []
    {
        status = sodium_init();
    });
    if (status < 0)
    {
        fail("operation_failed", "初始化公钥密码学库失败");
    }
}

std::span<const std::uint8_t> private_material(const secret::handle& value)
{
    const auto material = value->view();
    if (material.size() != 32)
    {
        fail("invalid_argument", "公钥算法私钥必须恰好为 32 字节");
    }
    return material;
}

secret::handle import_private(const byte_value& raw)
{
    if (raw->size() != 32)
    {
        fail("invalid_argument", "公钥算法私钥必须恰好为 32 字节");
    }
    return secret::from_bytes(raw);
}

void ed25519_keypair(const secret::handle& private_key,
                     std::array<std::uint8_t, 32>& public_value,
                     crypto::sensitive_buffer& expanded)
{
    ready();
    const auto seed = private_material(private_key);
    if (crypto_sign_seed_keypair(public_value.data(), expanded.data(),
                                 seed.data()) != 0)
    {
        fail("operation_failed", "生成 Ed25519 公钥失败");
    }
}

} // namespace

secret::handle ed25519_generate()
{
    return secret::random(32);
}

secret::handle ed25519_import_seed(const byte_value& seed)
{
    return import_private(seed);
}

byte_value ed25519_public(const secret::handle& private_key)
{
    std::array<std::uint8_t, 32> public_value{};
    crypto::sensitive_buffer expanded(64);
    ed25519_keypair(private_key, public_value, expanded);
    return make_bytes({public_value.begin(), public_value.end()});
}

byte_value ed25519_sign(const secret::handle& private_key,
                        const byte_value& message)
{
    std::array<std::uint8_t, 32> public_value{};
    crypto::sensitive_buffer expanded(64);
    ed25519_keypair(private_key, public_value, expanded);
    std::array<std::uint8_t, 64> signature{};
    if (crypto_sign_detached(signature.data(), nullptr,
            crypto::byte_data(message), message->size(), expanded.data()) != 0)
    {
        fail("operation_failed", "生成 Ed25519 签名失败");
    }
    return make_bytes({signature.begin(), signature.end()});
}

bool ed25519_verify(const byte_value& public_value, const byte_value& message,
                    const byte_value& signature)
{
    if (public_value->size() != 32 || signature->size() != 64)
    {
        fail("invalid_argument", "Ed25519 公钥或签名长度无效");
    }
    ready();
    return crypto_sign_verify_detached(signature->data(),
        crypto::byte_data(message), message->size(), public_value->data()) == 0;
}

secret::handle x25519_generate()
{
    return secret::random(32);
}

secret::handle x25519_import_private(const byte_value& raw)
{
    return import_private(raw);
}

byte_value x25519_public(const secret::handle& private_key)
{
    ready();
    const auto material = private_material(private_key);
    std::array<std::uint8_t, 32> public_value{};
    if (crypto_scalarmult_curve25519_base(public_value.data(),
                                           material.data()) != 0)
    {
        fail("operation_failed", "生成 X25519 公钥失败");
    }
    return make_bytes({public_value.begin(), public_value.end()});
}

secret::handle x25519_derive(const secret::handle& private_key,
                            const byte_value& peer_public,
                            const byte_value& salt, const byte_value& info)
{
    ready();
    const auto material = private_material(private_key);
    if (peer_public->size() != 32)
    {
        fail("invalid_argument", "X25519 对端公钥必须恰好为 32 字节");
    }
    crypto::sensitive_buffer shared(32);
    if (crypto_scalarmult_curve25519(shared.data(), material.data(),
                                     peer_public->data()) != 0)
    {
        fail("invalid_key", "X25519 对端公钥无效或产生低阶共享值");
    }
    const auto* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!md)
    {
        fail("operation_failed", "HKDF-SHA256 不可用");
    }
    auto derived = std::make_shared<secret::buffer>(32);
    if (mbedtls_hkdf(md, crypto::byte_data(salt), salt->size(),
            shared.data(), shared.size(), crypto::byte_data(info), info->size(),
            derived->writable().data(), 32) != 0)
    {
        fail("operation_failed", "派生 X25519 会话密钥失败");
    }
    return derived;
}

} // namespace tx_generated::public_key
