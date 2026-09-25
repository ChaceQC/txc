#include "backend/cpp/runtime_abi_internal.hpp"
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

extern "C" int txrt_encoding_encode(const void* text, const void* name,
                                      void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& source = *static_cast<const std::string*>(text);
        const auto selected = selected_encoding(name);
        if (source.size() > static_cast<std::size_t>(
                std::numeric_limits<int>::max()))
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "size_limit", "待编码文本超出转换长度上限"});
        }
        try
        {
            (void)tx_generated::detail::utf8_to_wide(source);
        }
        catch (const std::runtime_error& error)
        {
            throw tx_generated::runtime_failure({tx::error_kind::parse,
                "invalid_encoding", error.what()});
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
    });
}

extern "C" int txrt_encoding_decode(const void* value, const void* name,
                                      void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto selected = selected_encoding(name);
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
    });
}
