#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/public_key.hpp"

#include <any>

namespace
{

const tx_generated::secret::handle& secret_argument(const void* value)
{
    return std::any_cast<const tx_generated::secret::handle&>(
        *static_cast<const std::any*>(value));
}

const tx_generated::byte_value& byte_argument(const void* value)
{
    return tx_generated::bytes_of(*static_cast<const std::any*>(value));
}

template<class operation>
int value_result(void** result, operation&& run) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(run());
    });
}

} // namespace

extern "C" int txrt_public_key_ed25519_generate(void** result) noexcept
{
    return value_result(result, []
    {
        return tx_generated::public_key::ed25519_generate();
    });
}

extern "C" int txrt_public_key_ed25519_import_seed(const void* seed,
                                                    void** result) noexcept
{
    return value_result(result, [&]
    {
        return tx_generated::public_key::ed25519_import_seed(
            byte_argument(seed));
    });
}

extern "C" int txrt_public_key_ed25519_public(const void* private_key,
                                               void** result) noexcept
{
    return value_result(result, [&]
    {
        return tx_generated::public_key::ed25519_public(
            secret_argument(private_key));
    });
}

extern "C" int txrt_public_key_ed25519_sign(const void* private_key,
    const void* message, void** result) noexcept
{
    return value_result(result, [&]
    {
        return tx_generated::public_key::ed25519_sign(
            secret_argument(private_key), byte_argument(message));
    });
}

extern "C" int txrt_public_key_ed25519_verify(const void* public_value,
    const void* message, const void* signature, bool* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::public_key::ed25519_verify(
            byte_argument(public_value), byte_argument(message),
            byte_argument(signature));
    });
}

extern "C" int txrt_public_key_x25519_generate(void** result) noexcept
{
    return value_result(result, []
    {
        return tx_generated::public_key::x25519_generate();
    });
}

extern "C" int txrt_public_key_x25519_import_private(const void* raw,
                                                       void** result) noexcept
{
    return value_result(result, [&]
    {
        return tx_generated::public_key::x25519_import_private(
            byte_argument(raw));
    });
}

extern "C" int txrt_public_key_x25519_public(const void* private_key,
                                               void** result) noexcept
{
    return value_result(result, [&]
    {
        return tx_generated::public_key::x25519_public(
            secret_argument(private_key));
    });
}

extern "C" int txrt_public_key_x25519_derive(const void* private_key,
    const void* peer_public, const void* salt, const void* info,
    void** result) noexcept
{
    return value_result(result, [&]
    {
        return tx_generated::public_key::x25519_derive(
            secret_argument(private_key), byte_argument(peer_public),
            byte_argument(salt), byte_argument(info));
    });
}
