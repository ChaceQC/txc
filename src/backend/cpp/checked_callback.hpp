#pragma once

#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/closure.hpp"

#include <functional>
#include <type_traits>

namespace tx_generated
{

// 回调 ABI 由调用方的具体签名选择；这里只管理 TX 错误状态的消费边界。
template<class result_type, class... arguments>
result_type invoke_typed_callback(const void* operation, arguments... values)
{
    const auto& closure = std::any_cast<const closure_handle&>(
        *static_cast<const std::any*>(operation));
    if (!closure.data().target)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_callback", "回调为空"});
    }
    auto& context = detail::current_runtime_context();
    struct propagation_guard
    {
        detail::runtime_context& context;
        bool previous;
        ~propagation_guard()
        {
            context.propagate_errors = previous;
        }
    } guard{context, context.propagate_errors};
    context.propagate_errors = true;
    const auto check = [&]
    {
        if (context.last_error_kind != tx::error_kind::none)
        {
            error_info error{context.last_error_kind, context.last_error_code,
                             context.last_error};
            context.last_error_kind = tx::error_kind::none;
            throw runtime_failure(std::move(error));
        }
    };
    using callback_type = result_type (*)(void*, arguments...);
    const auto callback = reinterpret_cast<callback_type>(
        const_cast<void*>(closure.data().target));
    if constexpr (std::is_void_v<result_type>)
    {
        callback(const_cast<void*>(operation), values...);
        check();
    }
    else
    {
        auto result = callback(const_cast<void*>(operation), values...);
        check();
        return result;
    }
}

} // namespace tx_generated
