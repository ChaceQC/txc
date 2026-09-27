#include "backend/cpp/serde_abi.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/serde.hpp"

#include <any>
#include <string>

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_serde_serialize_json(const char* schema,
    const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::serde_serialize_json(
            schema, *static_cast<const std::any*>(value)));
    });
}

extern "C" int txrt_serde_deserialize_json(const char* schema,
    const void* text, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::serde_deserialize_json(
            schema, *static_cast<const std::string*>(text)));
    });
}

extern "C" int txrt_serde_serialize_cbor(const char* schema,
    const void* value, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::serde_serialize_cbor(
            schema, *static_cast<const std::any*>(value)));
    });
}

extern "C" int txrt_serde_deserialize_cbor(const char* schema,
    const void* bytes, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::serde_deserialize_cbor(
            schema, tx_generated::bytes_of(
                *static_cast<const std::any*>(bytes))));
    });
}
