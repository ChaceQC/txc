#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/encoding_incremental.hpp"

#include <any>
#include <cstdint>
#include <string>

namespace
{

const std::string& text_value(const void* value)
{
    return *static_cast<const std::string*>(value);
}

const tx_generated::encoding_decoder& decoder_value(const void* value)
{
    return std::any_cast<const tx_generated::encoding_decoder&>(
        *static_cast<const std::any*>(value));
}

const tx_generated::encoding_encoder& encoder_value(const void* value)
{
    return std::any_cast<const tx_generated::encoding_encoder&>(
        *static_cast<const std::any*>(value));
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" int txrt_encoding_new_decoder(const void* encoding,
    const void* policy, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::new_decoder(
            text_value(encoding), text_value(policy)));
    });
}

extern "C" int txrt_encoding_decode_chunk(const void* source,
    const void* data, bool eof, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(tx_generated::decode_chunk(
            decoder_value(source), tx_generated::bytes_of(
                *static_cast<const std::any*>(data)), eof));
    });
}

extern "C" int txrt_encoding_decoder_replacements(const void* source,
    std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::decoder_replacements(decoder_value(source));
    });
}

extern "C" int txrt_encoding_new_encoder(const void* encoding,
    const void* policy, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::new_encoder(
            text_value(encoding), text_value(policy)));
    });
}

extern "C" int txrt_encoding_encode_chunk(const void* target,
    const void* text, bool eof, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(tx_generated::encode_chunk(
            encoder_value(target), text_value(text), eof));
    });
}

extern "C" int txrt_encoding_encoder_replacements(const void* target,
    std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::encoder_replacements(encoder_value(target));
    });
}
