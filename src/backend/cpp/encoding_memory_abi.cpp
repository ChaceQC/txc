#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/encoding_memory_abi.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/encoding.hpp"

#include <any>
#include <limits>
#include <string>
#include <vector>

namespace
{

tx_generated::detail::text_encoding selected_encoding(const void* name)
{
    try
    {
        return tx_generated::detail::parse_encoding(
            *static_cast<const std::string*>(name));
    }
    catch (const std::runtime_error& error)
    {
        throw tx_generated::runtime_failure({tx::error_kind::parse,
            "unknown_encoding", error.what()});
    }
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

namespace
{

void encode_memory(const void* text, tx::text_encoding selected, void** result)
{
    const auto& source = *static_cast<const std::string*>(text);
    if (source.size() > static_cast<std::size_t>(
            std::numeric_limits<int>::max()))
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "size_limit", "待编码文本超出转换长度上限"});
    }
    try
    {
        tx_generated::detail::validate_utf8(source);
    }
    catch (const std::runtime_error& error)
    {
        throw tx_generated::runtime_failure({tx::error_kind::parse,
            "invalid_encoding", error.what()});
    }
    if (selected == tx_generated::detail::text_encoding::utf8 ||
        selected == tx_generated::detail::text_encoding::utf8_sig)
    {
        std::vector<std::uint8_t> bytes;
        bytes.reserve(source.size() + (selected ==
            tx_generated::detail::text_encoding::utf8_sig ? 3 : 0));
        if (selected == tx_generated::detail::text_encoding::utf8_sig)
        {
            bytes.insert(bytes.end(), {0xef, 0xbb, 0xbf});
        }
        bytes.insert(bytes.end(), source.begin(), source.end());
        *result = make_handle<std::any>(tx_generated::make_bytes(std::move(bytes)));
        return;
    }
    std::string encoded;
    try
    {
        encoded = tx_generated::detail::encode_text(source, selected);
    }
    catch (const std::runtime_error& error)
    {
        throw tx_generated::runtime_failure({tx::error_kind::parse,
            "unrepresentable_character", error.what()});
    }
    *result = make_handle<std::any>(tx_generated::make_bytes(
        {encoded.begin(), encoded.end()}));
}

void decode_memory(const void* value, tx::text_encoding selected, void** result)
{
    const auto& data = *tx_generated::bytes_of(
        *static_cast<const std::any*>(value));
    if (data.size() > static_cast<std::size_t>(
            std::numeric_limits<int>::max()))
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "size_limit", "待解码字节值超出转换长度上限"});
    }
    const std::string_view source = data.empty() ? std::string_view{}
        : std::string_view(reinterpret_cast<const char*>(data.data()),
                           data.size());
    try
    {
        *result = make_handle<std::string>(
            tx_generated::detail::decode_text(source, selected));
    }
    catch (const std::runtime_error& error)
    {
        throw tx_generated::runtime_failure({tx::error_kind::parse,
            "invalid_encoding", error.what()});
    }
}

} // namespace

extern "C" int txrt_encoding_encode(const void* source, const void* name, void** result) noexcept
{
    return invoke_checked([&]
    {
        encode_memory(source, selected_encoding(name), result);
    });
}

extern "C" int txrt_encoding_encode_known(const void* source, std::int64_t selected, void** result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        encode_memory(source, static_cast<tx::text_encoding>(selected), result);
    });
}

extern "C" int txrt_encoding_decode(const void* source, const void* name, void** result) noexcept
{
    return invoke_checked([&]
    {
        decode_memory(source, selected_encoding(name), result);
    });
}

extern "C" int txrt_encoding_decode_known(const void* source, std::int64_t selected, void** result) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        decode_memory(source, static_cast<tx::text_encoding>(selected), result);
    });
}
