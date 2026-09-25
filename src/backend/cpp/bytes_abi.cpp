#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/bytes.hpp"

#include <any>
#include <string>

namespace
{

const tx_generated::byte_value& byte_argument(const void* value)
{
    return tx_generated::bytes_of(*static_cast<const std::any*>(value));
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_bytes_empty(void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::make_bytes({}));
    });
}

extern "C" int txrt_bytes_from_vector(const void* values, void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& vector = std::any_cast<const tx_generated::int_vector&>(
            *static_cast<const std::any*>(values));
        *result = make_handle<std::any>(tx_generated::bytes_from_vector(vector));
    });
}

extern "C" int txrt_bytes_to_vector(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::bytes_to_vector(
            byte_argument(value)));
    });
}

extern "C" int txrt_bytes_concat(const void* left, const void* right,
                                   void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::bytes_concat(
            byte_argument(left), byte_argument(right)));
    });
}

extern "C" int txrt_bytes_slice(const void* value, std::int64_t start,
                                  std::int64_t end, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::bytes_slice(
            byte_argument(value), start, end));
    });
}

extern "C" int txrt_bytes_to_hex(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::bytes_to_hex(
            byte_argument(value)));
    });
}

extern "C" int txrt_bytes_from_hex(const void* text, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::bytes_from_hex(
            *static_cast<const std::string*>(text)));
    });
}

extern "C" int txrt_bytes_to_base64(const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::bytes_to_base64(
            byte_argument(value)));
    });
}

extern "C" int txrt_bytes_from_base64(const void* text, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::bytes_from_base64(
            *static_cast<const std::string*>(text)));
    });
}

extern "C" int txrt_bytes_at(const void* value, std::int64_t index,
                               std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::bytes_at(byte_argument(value), index);
    });
}

extern "C" int txrt_bytes_equal(const void* left, const void* right,
                                  bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = *byte_argument(left) == *byte_argument(right);
    });
}
