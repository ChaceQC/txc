#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/runtime_abi.hpp"

#include <stdexcept>
#include <string_view>

extern "C" int txrt_str_concat_literal(const void* value, const char* bytes,
    std::size_t length, bool prepend, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        if (length == 0)
        {
            *result = tx_generated::detail::retain_text_handle(value);
            return;
        }
        const auto& text = *static_cast<const std::string*>(value);
        std::string combined;
        if (length > combined.max_size() - text.size())
        {
            throw std::length_error("字符串拼接结果过长");
        }
        combined.reserve(text.size() + length);
        const std::string_view literal(bytes, length);
        combined.append(prepend ? literal : std::string_view(text));
        combined.append(prepend ? std::string_view(text) : literal);
        *result = tx_generated::detail::make_handle<std::string>(std::move(combined));
    });
}
