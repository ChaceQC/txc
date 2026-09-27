#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/secret.hpp"

#include <any>
#include <cstdint>
#include <utility>

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
        *result = tx_generated::detail::make_handle<std::any>(
            std::forward<operation>(run)());
    });
}

} // namespace

extern "C" int txrt_secret_from_bytes(const void* data, void** result) noexcept
{
    return value_result(result, [&]
    {
        return tx_generated::secret::from_bytes(byte_argument(data));
    });
}

extern "C" int txrt_secret_random(std::int64_t count, void** result) noexcept
{
    return value_result(result, [&]
    {
        return tx_generated::secret::random(count);
    });
}

extern "C" int txrt_secret_to_bytes(const void* value, void** result) noexcept
{
    return value_result(result, [&]
    {
        return tx_generated::secret::to_bytes(secret_argument(value));
    });
}

extern "C" int txrt_secret_size(const void* value,
                                std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::secret::size(secret_argument(value));
    });
}

extern "C" int txrt_secret_equal(const void* left, const void* right,
                                 bool* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::secret::equal(secret_argument(left),
                                              secret_argument(right));
    });
}

extern "C" int txrt_secret_close(const void* value) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        tx_generated::secret::close(secret_argument(value));
    });
}
