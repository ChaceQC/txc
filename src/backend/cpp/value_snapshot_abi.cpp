#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_abi.hpp"
#include "stdlib/stdlib.hpp"

#include <any>
#include <bit>
#include <cstdint>
#include <stdexcept>

namespace
{

std::any scalar_snapshot_value(std::uint8_t kind, std::uint64_t bits,
                               const void* fallback)
{
    switch (kind)
    {
    case 0: return *static_cast<const std::any*>(fallback);
    case 1: return {};
    case 2: return std::bit_cast<std::int64_t>(bits);
    case 3: return std::bit_cast<double>(bits);
    case 4: return bits != 0;
    default: throw std::runtime_error("无效的标量快照");
    }
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" bool txrt_value_snapshot_scalar(const void* value,
                                             std::uint8_t* kind,
                                             std::uint64_t* bits) noexcept
{
    const auto& item = *static_cast<const std::any*>(value);
    if (!item.has_value())
    {
        *kind = 1;
        *bits = 0;
    }
    else if (const auto* integer = std::any_cast<std::int64_t>(&item))
    {
        *kind = 2;
        *bits = std::bit_cast<std::uint64_t>(*integer);
    }
    else if (const auto* number = std::any_cast<double>(&item))
    {
        *kind = 3;
        *bits = std::bit_cast<std::uint64_t>(*number);
    }
    else if (const auto* boolean = std::any_cast<bool>(&item))
    {
        *kind = 4;
        *bits = *boolean ? 1 : 0;
    }
    else
    {
        *kind = 0;
        *bits = 0;
        return false;
    }
    return true;
}

extern "C" int txrt_value_snapshot_box(std::uint8_t kind,
                                         std::uint64_t bits,
                                         const void* fallback,
                                         void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(
            scalar_snapshot_value(kind, bits, fallback));
    });
}

extern "C" std::int64_t txrt_value_snapshot_to_i64_fast(
    std::uint8_t kind, std::uint64_t bits, const void* fallback) noexcept
{
    if (kind == 2)
    {
        return std::bit_cast<std::int64_t>(bits);
    }
    std::int64_t result = 0;
    txrt_require_success(invoke_checked([&]
    {
        result = tx_generated::tx_to_int(
            scalar_snapshot_value(kind, bits, fallback));
    }));
    return result;
}

extern "C" double txrt_value_snapshot_to_f64_fast(
    std::uint8_t kind, std::uint64_t bits, const void* fallback) noexcept
{
    if (kind == 3)
    {
        return std::bit_cast<double>(bits);
    }
    double result = 0.0;
    txrt_require_success(invoke_checked([&]
    {
        result = tx_generated::tx_to_float(
            scalar_snapshot_value(kind, bits, fallback));
    }));
    return result;
}

extern "C" bool txrt_value_snapshot_to_bool_fast(
    std::uint8_t kind, std::uint64_t bits, const void* fallback) noexcept
{
    if (kind == 4)
    {
        return bits != 0;
    }
    bool result = false;
    txrt_require_success(invoke_checked([&]
    {
        const auto value = scalar_snapshot_value(kind, bits, fallback);
        if (value.type() != typeid(bool))
        {
            throw std::runtime_error("数组元素不是 bool");
        }
        result = std::any_cast<bool>(value);
    }));
    return result;
}
