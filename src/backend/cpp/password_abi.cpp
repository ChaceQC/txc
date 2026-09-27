#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/password.hpp"

#include <any>
#include <cstdint>
#include <string>

namespace
{

const tx_generated::secret::handle& secret_argument(const void* value)
{
    return std::any_cast<const tx_generated::secret::handle&>(
        *static_cast<const std::any*>(value));
}

const std::string& text_argument(const void* value)
{
    return *static_cast<const std::string*>(value);
}

} // namespace

extern "C" int txrt_password_hash_password(const void* value,
                                            void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::password::hash_password(secret_argument(value)));
    });
}

extern "C" int txrt_password_hash_password_with_params(const void* value,
    std::int64_t memory_kib, std::int64_t iterations,
    std::int64_t parallelism, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::password::hash_password_with_params(
                secret_argument(value), memory_kib, iterations, parallelism));
    });
}

extern "C" int txrt_password_verify_password(const void* value,
    const void* encoded, bool* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::password::verify_password(
            secret_argument(value), text_argument(encoded));
    });
}

extern "C" int txrt_password_needs_rehash(const void* encoded,
    std::int64_t memory_kib, std::int64_t iterations,
    std::int64_t parallelism, bool* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::password::needs_rehash(text_argument(encoded),
            memory_kib, iterations, parallelism);
    });
}
