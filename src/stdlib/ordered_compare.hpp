#pragma once

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/value_abi.hpp"
#include "stdlib/closure.hpp"
#include "stdlib/container_scalar.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace tx_generated
{

inline void require_ordered_callback(int status)
{
    if (status != 0)
    {
        throw runtime_failure({detail::current_runtime_context().last_error_kind,
            detail::current_runtime_context().last_error_code, detail::current_runtime_context().last_error});
    }
}

[[nodiscard]] inline std::any copy_ordered_key(const std::any& value)
{
    void* copied = nullptr;
    require_ordered_callback(txrt_value_deep_copy(&value, &copied));
    const std::unique_ptr<std::any, decltype(&txrt_value_release)> owned(
        static_cast<std::any*>(copied), txrt_value_release);
    return *owned;
}

template<class key_type>
[[nodiscard]] key_type copy_ordered_key(const key_type& value)
{
    return value;
}

template<class key_type>
struct ordered_compare
{
    using less_callback = int (*)(const void*, const void*, bool*);

    const void* less = nullptr;
    std::any closure;
    std::shared_ptr<bool> invoking = std::make_shared<bool>(false);

    ordered_compare() = default;

    ordered_compare(const void* less, std::any closure)
        : less(less), closure(std::move(closure))
    {
    }

    [[nodiscard]] bool operator()(const key_type& left,
                                  const key_type& right) const
    {
        require_ordered_key(left);
        require_ordered_key(right);
        if (closure.has_value())
        {
            return invoke_closure(left, right) < 0;
        }
        if constexpr (std::is_same_v<key_type, std::any>)
        {
            if (!less)
            {
                throw std::runtime_error("有序结构体键缺少 operator <");
            }
            const auto copied_left = copy_ordered_key(left);
            const auto copied_right = copy_ordered_key(right);
            bool result = false;
            callback_guard guard(*invoking);
            require_ordered_callback(reinterpret_cast<less_callback>(
                const_cast<void*>(less))(&copied_left, &copied_right, &result));
            return result;
        }
        else
        {
            return scalar_value(left) < scalar_value(right);
        }
    }

private:
    struct callback_guard
    {
        bool& active;

        explicit callback_guard(bool& value) : active(value)
        {
            if (active)
            {
                throw std::runtime_error("有序容器比较器不能递归访问同一容器");
            }
            active = true;
        }

        ~callback_guard()
        {
            active = false;
        }
    };

    [[nodiscard]] std::int64_t invoke_closure(const key_type& left,
                                                const key_type& right) const
    {
        const auto& state = std::any_cast<const closure_handle&>(closure).data();
        callback_guard guard(*invoking);
        std::int64_t result = 0;
        if constexpr (std::is_same_v<key_type, std::int64_t>)
        {
            using target = std::int64_t (*)(const void*, std::int64_t, std::int64_t);
            result = reinterpret_cast<target>(const_cast<void*>(state.target))(
                &closure, left, right);
        }
        else if constexpr (std::is_same_v<key_type, double>)
        {
            using target = std::int64_t (*)(const void*, double, double);
            result = reinterpret_cast<target>(const_cast<void*>(state.target))(
                &closure, left, right);
        }
        else if constexpr (std::is_same_v<key_type, std::uint8_t>)
        {
            using target = std::int64_t (*)(const void*, bool, bool);
            result = reinterpret_cast<target>(const_cast<void*>(state.target))(
                &closure, left != 0, right != 0);
        }
        else if constexpr (std::is_same_v<key_type, text_reference>)
        {
            using target = std::int64_t (*)(const void*, const void*, const void*);
            auto* left_copy = detail::copy_text_handle(left.handle());
            auto* right_copy = detail::copy_text_handle(right.handle());
            result = reinterpret_cast<target>(const_cast<void*>(state.target))(
                &closure, left_copy, right_copy);
        }
        else
        {
            using target = std::int64_t (*)(const void*, const void*, const void*);
            void* left_copy = nullptr;
            void* right_copy = nullptr;
            require_ordered_callback(txrt_value_deep_copy(&left, &left_copy));
            std::unique_ptr<std::any, decltype(&txrt_value_release)> left_owned(
                static_cast<std::any*>(left_copy), txrt_value_release);
            require_ordered_callback(txrt_value_deep_copy(&right, &right_copy));
            std::unique_ptr<std::any, decltype(&txrt_value_release)> right_owned(
                static_cast<std::any*>(right_copy), txrt_value_release);
            result = reinterpret_cast<target>(const_cast<void*>(state.target))(
                &closure, left_owned.release(), right_owned.release());
        }
        require_ordered_callback(txrt_error_status());
        return result;
    }
};

} // namespace tx_generated
