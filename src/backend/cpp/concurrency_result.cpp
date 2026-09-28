#include "backend/cpp/concurrency_result.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"

#include <memory>
#include <stdexcept>

namespace tx_generated
{

concurrent_result invoke_concurrent_callback(const void* target,
    const void* callback, std::int64_t kind)
{
    switch (kind)
    {
    case 0:
        reinterpret_cast<void (*)(const void*)>(target)(callback);
        return {};
    case 1:
        return reinterpret_cast<std::int64_t (*)(const void*)>(target)(callback);
    case 2:
        return reinterpret_cast<double (*)(const void*)>(target)(callback);
    case 3:
        return reinterpret_cast<bool (*)(const void*)>(target)(callback);
    case 4:
    {
        using text_owner = std::unique_ptr<std::string,
            decltype(&detail::destroy_handle<std::string>)>;
        text_owner value(reinterpret_cast<std::string* (*)(const void*)>(
            target)(callback), &detail::destroy_handle<std::string>);
        if (!value)
        {
            throw std::runtime_error("跨线程文本结果为空");
        }
        return *value;
    }
    case 5:
    {
        using value_owner = std::unique_ptr<std::any,
            decltype(&detail::destroy_handle<std::any>)>;
        value_owner value(reinterpret_cast<std::any* (*)(const void*)>(
            target)(callback), &detail::destroy_handle<std::any>);
        if (!value)
        {
            throw std::runtime_error("跨线程复合结果为空");
        }
        return *value;
    }
    default:
        throw std::runtime_error("跨线程结果类型无效");
    }
}

} // namespace tx_generated
