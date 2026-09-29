#include "backend/cpp/parse_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/parse.hpp"

namespace
{

std::string_view error_field(std::int64_t error, std::int64_t field) noexcept
{
    const auto text = tx_generated::parse_error_text_of(static_cast<tx_generated::parse_error>(error));
    return field == 0 ? text.kind : field == 1 ? text.code : text.message;
}

} // namespace

extern "C" int txrt_parse_error_field_context(void* context, std::int64_t error,
    std::int64_t field, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(error_field(error, field));
    }, tx::error_kind::runtime, static_cast<tx_generated::detail::runtime_context*>(context));
}

extern "C" std::int64_t txrt_parse_error_field_length(std::int64_t error,
    std::int64_t field) noexcept
{
    std::int64_t length = 0;
    // 错误字段全部来自编译期 UTF-8 常量，仍按 Unicode 标量而非字节数返回长度。
    for (const unsigned char byte : error_field(error, field))
    {
        length += (byte & 0xc0) != 0x80;
    }
    return length;
}
