#pragma once

#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/closure.hpp"

#include <functional>
#include <type_traits>

namespace tx_generated
{

// 回调 ABI 由调用方的具体签名选择；这里只管理 TX 错误状态的消费边界。
// 绑定只用于持有原回调参数的同步调用期间，不跨线程或跨调用保存借用视图。
template<class result_type, class... arguments>
class bound_typed_callback
{
public:
    explicit bound_typed_callback(const void* operation)
        : operation_(operation), context_(detail::current_runtime_context())
    {
        if (!operation)
        {
            throw runtime_failure({tx::error_kind::runtime, "invalid_callback", "回调为空"});
        }
        const auto& closure = std::any_cast<const closure_handle&>(
            *static_cast<const std::any*>(operation));
        if (!closure.data().target)
        {
            throw runtime_failure({tx::error_kind::runtime, "invalid_callback", "回调为空"});
        }
        view_ = &closure.data().view;
        target_ = closure.data().target;
    }

    result_type operator()(arguments... values) const
    {
        auto& context = context_;
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
        // 内部入口与原生适配器具有相同的参数拥有规则，直接传入当前执行
        // 线程的 context 和稳定 view，避免适配器重复提取 TLS 与闭包句柄。
        using internal_callback_type = result_type (*)(void*, const void*, arguments...);
        using callback_type = result_type (*)(void*, arguments...);
        const auto& view = *view_;
        const auto invoke = [&]() -> result_type
        {
            if (view.code)
            {
                return reinterpret_cast<internal_callback_type>(
                    const_cast<void*>(view.code))(&context, &view, values...);
            }
            return reinterpret_cast<callback_type>(
                    const_cast<void*>(target_))(
                    const_cast<void*>(operation_), values...);
        };
        if constexpr (std::is_void_v<result_type>)
        {
            invoke();
            check();
        }
        else
        {
            auto result = invoke();
            check();
            return result;
        }
    }

private:
    const void* operation_;
    detail::runtime_context& context_;
    const closure_view* view_ = nullptr;
    const void* target_ = nullptr;
};

template<class result_type, class... arguments>
result_type invoke_typed_callback(const void* operation, arguments... values)
{
    return bound_typed_callback<result_type, arguments...>(operation)(values...);
}

} // namespace tx_generated
