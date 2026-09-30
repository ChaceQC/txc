#include "backend/cpp/concurrency_result.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"

#include <memory>
#include <stdexcept>

namespace tx_generated
{

concurrent_result invoke_concurrent_callback(const void* target,
    const void* callback, std::int64_t kind)
{
    auto* callback_target = const_cast<void*>(target);
    switch (kind)
    {
    case 0:
        reinterpret_cast<void (*)(const void*)>(callback_target)(callback);
        return {};
    case 1:
        return reinterpret_cast<std::int64_t (*)(const void*)>(callback_target)(callback);
    case 2:
        return reinterpret_cast<double (*)(const void*)>(callback_target)(callback);
    case 3:
        return reinterpret_cast<bool (*)(const void*)>(callback_target)(callback);
    case 4:
    {
        using text_owner = std::unique_ptr<detail::text_handle_record,
            decltype(&detail::destroy_handle<detail::text_handle_record>)>;
        text_owner value(reinterpret_cast<detail::text_handle_record* (*)(const void*)>(
            callback_target)(callback), &detail::destroy_handle<detail::text_handle_record>);
        if (!value)
        {
            // TX 回调失败时以空指针返回，原始错误已经留在当前上下文。
            if (detail::current_runtime_context().last_error_kind ==
                tx::error_kind::none)
            {
                throw std::runtime_error("跨线程文本结果为空");
            }
            return {};
        }
        return detail::text_value(value.get());
    }
    case 5:
    {
        using value_owner = std::unique_ptr<std::any,
            decltype(&detail::destroy_handle<std::any>)>;
        value_owner value(reinterpret_cast<std::any* (*)(const void*)>(
            callback_target)(callback), &detail::destroy_handle<std::any>);
        if (!value)
        {
            if (detail::current_runtime_context().last_error_kind ==
                tx::error_kind::none)
            {
                throw std::runtime_error("跨线程复合结果为空");
            }
            return {};
        }
        return *value;
    }
    default:
        throw std::runtime_error("跨线程结果类型无效");
    }
}

} // namespace tx_generated
