#pragma once

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/runtime_abi.hpp"
#include "stdlib/closure.hpp"
#include "stdlib/ordered_compare.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace tx_generated
{

template<class value_type>
struct algorithm_callback_value
{
    using abi_type = value_type;

    explicit algorithm_callback_value(const value_type& value) : value(value)
    {
    }

    [[nodiscard]] abi_type take() const
    {
        return value;
    }

    [[nodiscard]] static value_type result(abi_type value)
    {
        return value;
    }

    value_type value;
};

template<>
struct algorithm_callback_value<std::uint8_t>
{
    using abi_type = bool;

    explicit algorithm_callback_value(std::uint8_t value) : value(value != 0)
    {
    }

    [[nodiscard]] bool take() const
    {
        return value;
    }

    [[nodiscard]] static std::uint8_t result(bool value)
    {
        return static_cast<std::uint8_t>(value);
    }

    bool value;
};

template<>
struct algorithm_callback_value<text_reference>
{
    using abi_type = const void*;

    explicit algorithm_callback_value(const text_reference& value)
        : value(detail::copy_text_handle(value.handle()))
    {
    }

    algorithm_callback_value(const algorithm_callback_value&) = delete;

    algorithm_callback_value(algorithm_callback_value&& other) noexcept
        : value(std::exchange(other.value, nullptr))
    {
    }

    ~algorithm_callback_value()
    {
        txrt_str_release(value);
    }

    [[nodiscard]] const void* take()
    {
        return std::exchange(value, nullptr);
    }

    [[nodiscard]] static text_reference result(const void* value)
    {
        text_reference text(value);
        txrt_str_release(const_cast<void*>(value));
        return text;
    }

    void* value;
};

template<>
struct algorithm_callback_value<std::any>
{
    using abi_type = const void*;

    explicit algorithm_callback_value(const std::any& value)
        : value(detail::make_handle<std::any>(value))
    {
    }

    algorithm_callback_value(const algorithm_callback_value&) = delete;

    algorithm_callback_value(algorithm_callback_value&& other) noexcept
        : value(std::exchange(other.value, nullptr))
    {
    }

    ~algorithm_callback_value()
    {
        txrt_value_release(value);
    }

    [[nodiscard]] const void* take()
    {
        return std::exchange(value, nullptr);
    }

    [[nodiscard]] static std::any result(const void* value)
    {
        const std::unique_ptr<std::any, decltype(&txrt_value_release)> owned(
            static_cast<std::any*>(const_cast<void*>(value)), txrt_value_release);
        return *owned;
    }

    void* value;
};

template<>
struct algorithm_callback_value<byte_value>
{
    using abi_type = const void*;

    explicit algorithm_callback_value(const byte_value& input)
        : value(detail::make_handle<std::any>(input))
    {
    }

    algorithm_callback_value(const algorithm_callback_value&) = delete;

    algorithm_callback_value(algorithm_callback_value&& other) noexcept
        : value(std::exchange(other.value, nullptr))
    {
    }

    ~algorithm_callback_value()
    {
        txrt_value_release(value);
    }

    [[nodiscard]] const void* take()
    {
        return std::exchange(value, nullptr);
    }

    [[nodiscard]] static byte_value result(const void* value)
    {
        const std::unique_ptr<std::any, decltype(&txrt_value_release)> owned(
            static_cast<std::any*>(const_cast<void*>(value)), txrt_value_release);
        return std::any_cast<byte_value>(*owned);
    }

    void* value;
};

template<class result_type, class... argument_types>
[[nodiscard]] result_type invoke_algorithm_callback(
    const std::any& callback, const argument_types&... arguments)
{
    const auto& state = std::any_cast<const closure_handle&>(callback).data();
    using target_type = typename algorithm_callback_value<result_type>::abi_type (*)(
        const void*, typename algorithm_callback_value<argument_types>::abi_type...);
    auto prepared = std::tuple<algorithm_callback_value<argument_types>...>(
        algorithm_callback_value<argument_types>(arguments)...);
    const auto target = reinterpret_cast<target_type>(
        const_cast<void*>(state.target));
    const auto raw = std::apply([&](auto&... values)
    {
        return target(&callback, values.take()...);
    }, prepared);
    const auto status = txrt_error_status();
    if (status != 0)
    {
        if constexpr (std::is_same_v<result_type, text_reference>)
        {
            txrt_str_release(const_cast<void*>(raw));
        }
        else if constexpr (std::is_same_v<result_type, std::any> ||
                           std::is_same_v<result_type, byte_value>)
        {
            txrt_value_release(const_cast<void*>(raw));
        }
        require_ordered_callback(status);
    }
    return algorithm_callback_value<result_type>::result(raw);
}

} // namespace tx_generated
