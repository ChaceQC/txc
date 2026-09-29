#include "stdlib/decimal.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"

#include <algorithm>
#include <any>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace tx_generated::decimal_math
{
namespace
{

[[noreturn]] void invalid_value()
{
    throw runtime_failure({tx::error_kind::runtime, "invalid_argument",
                           "十进制值的内部状态无效"});
}

value read(const void* source)
{
    const auto& holder = *static_cast<const std::any*>(source);
    const auto& object = std::any_cast<const class_handle&>(holder);
    if (!object || std::string_view(object->display_name) != "decimal" ||
        object->fields.size() != 3)
    {
        invalid_value();
    }
    const auto negative = std::any_cast<bool>(object->fields[0]);
    const auto& text = std::any_cast<const std::string&>(object->fields[1]);
    const auto scale = std::any_cast<std::int64_t>(object->fields[2]);
    if (scale < 0 || scale > 18 || text.empty() || text.size() > 38 ||
        (text.size() > 1 && text.front() == '0') ||
        !std::all_of(text.begin(), text.end(),
                     [](char digit) { return digit >= '0' && digit <= '9'; }))
    {
        invalid_value();
    }
    value result{negative, natural(text), static_cast<int>(scale)};
    if (result.digits.is_zero() && negative)
    {
        invalid_value();
    }
    return result;
}

void* make(value number, const char* type_name)
{
    auto object = std::make_shared<dynamic_class>();
    object->type_name = type_name;
    object->display_name = "decimal";
    object->owned_ancestors.push_back(type_name);
    object->ancestors = object->owned_ancestors.data();
    object->ancestor_count = 1;
    object->fields.emplace_back(number.negative);
    object->fields.emplace_back(number.digits.text());
    object->fields.emplace_back(static_cast<std::int64_t>(number.scale));
    register_class_gc(object);
    return detail::make_handle<std::any>(class_handle(std::move(object)));
}

} // namespace

} // namespace tx_generated::decimal_math

extern "C" int txrt_decimal_parse(const void* text, const char* type_name,
                                    void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::decimal_math::make(
            tx_generated::decimal_math::parse(
                tx_generated::detail::text_value(text)), type_name);
    });
}

extern "C" int txrt_decimal_from_int(std::int64_t number,
                                       const char* type_name,
                                       void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::decimal_math::make(
            tx_generated::decimal_math::from_integer(number), type_name);
    });
}

extern "C" int txrt_decimal_to_text(const void* source,
                                      void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::decimal_math::to_text(
                tx_generated::decimal_math::read(source)));
    });
}

extern "C" int txrt_decimal_scale(const void* source,
                                    std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::decimal_math::read(source).scale;
    });
}

extern "C" int txrt_decimal_quantize(const void* source, std::int64_t scale,
                                       const void* mode, const char* type_name,
                                       void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        if (scale < 0 || scale > 18)
        {
            throw tx_generated::runtime_failure({tx::error_kind::runtime,
                "invalid_argument", "目标小数位须位于 0～18"});
        }
        *result = tx_generated::decimal_math::make(
            tx_generated::decimal_math::quantize(
                tx_generated::decimal_math::read(source),
                static_cast<int>(scale),
                tx_generated::detail::text_value(mode)), type_name);
    });
}

#define TX_DECIMAL_BINARY(name, operation)                                   \
extern "C" int txrt_decimal_##name(const void* left, const void* right,     \
    std::int64_t scale, const void* mode, const char* type_name,              \
    void** result) noexcept                                                  \
{                                                                            \
    return tx_generated::detail::invoke_checked([&]                          \
    {                                                                        \
        if (scale < 0 || scale > 18)                                          \
        {                                                                    \
            throw tx_generated::runtime_failure({tx::error_kind::runtime,    \
                "invalid_argument", "目标小数位须位于 0～18"});             \
        }                                                                    \
        *result = tx_generated::decimal_math::make(                          \
            tx_generated::decimal_math::operation(                           \
                tx_generated::decimal_math::read(left),                      \
                tx_generated::decimal_math::read(right),                     \
                static_cast<int>(scale),                                     \
                tx_generated::detail::text_value(mode)), type_name);         \
    });                                                                      \
}

TX_DECIMAL_BINARY(add, add)
TX_DECIMAL_BINARY(sub, subtract)
TX_DECIMAL_BINARY(mul, multiply)
TX_DECIMAL_BINARY(div, divide)

#undef TX_DECIMAL_BINARY

extern "C" int txrt_decimal_compare(const void* left, const void* right,
                                      std::int64_t* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::decimal_math::compare(
            tx_generated::decimal_math::read(left),
            tx_generated::decimal_math::read(right));
    });
}
