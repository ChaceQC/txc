#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/runtime_abi.hpp"
#include "stdlib/closure.hpp"

#include <any>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace
{

const tx_generated::closure_state& state_of(const void* value)
{
    if (!value)
    {
        throw std::runtime_error("闭包引用为空");
    }
    return std::any_cast<const tx_generated::closure_handle&>(
        *static_cast<const std::any*>(value)).data();
}

template<class value_type>
int scalar_capture(const void* value, std::size_t index,
                   value_type* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        const auto& captures = state_of(value).captures;
        if (index >= captures.size())
        {
            throw std::out_of_range("闭包捕获索引越界");
        }
        *result = std::any_cast<value_type>(captures[index]);
    });
}

} // namespace

extern "C" int txrt_closure_new(const void* target, const char* type_name,
                                  void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        if (!target || !type_name)
        {
            throw std::runtime_error("闭包缺少静态目标或类型");
        }
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::closure_handle({target, type_name, {}, {}}));
    });
}

extern "C" int txrt_closure_bind(const void* parent, const void* target,
    const char* type_name, const void* const* captured,
    std::size_t count, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        (void)state_of(parent);
        if (!target || !type_name || (count != 0 && !captured))
        {
            throw std::runtime_error("闭包绑定参数无效");
        }
        tx_generated::closure_state state{target, type_name,
            *static_cast<const std::any*>(parent), {}};
        state.captures.reserve(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            if (!captured[index])
            {
                throw std::runtime_error("闭包捕获值为空");
            }
            state.captures.push_back(
                *static_cast<const std::any*>(captured[index]));
        }
        *result = tx_generated::detail::make_handle<std::any>(
            tx_generated::closure_handle(std::move(state)));
    });
}

extern "C" const void* txrt_closure_code(const void* value) noexcept
{
    const void* result = nullptr;
    txrt_require_success(tx_generated::detail::invoke_checked([&]
    {
        result = state_of(value).target;
    }));
    return result;
}

extern "C" int txrt_closure_parent(const void* value,
                                     void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::any>(
            state_of(value).parent);
    });
}

extern "C" int txrt_closure_parent_borrow(const void* value,
                                            const void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        // 生成的同步调用始终保留环境句柄，因此父闭包可借用至包装函数返回。
        *result = &state_of(value).parent;
    });
}

extern "C" int txrt_closure_capture(const void* value,
    std::size_t index, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        const auto& captures = state_of(value).captures;
        if (index >= captures.size())
        {
            throw std::out_of_range("闭包捕获索引越界");
        }
        *result = tx_generated::detail::make_handle<std::any>(captures[index]);
    });
}

extern "C" int txrt_closure_capture_i64(const void* value,
    std::size_t index, std::int64_t* result) noexcept
{
    return scalar_capture(value, index, result);
}

extern "C" int txrt_closure_capture_f64(const void* value,
    std::size_t index, double* result) noexcept
{
    return scalar_capture(value, index, result);
}

extern "C" int txrt_closure_capture_bool(const void* value,
    std::size_t index, bool* result) noexcept
{
    return scalar_capture(value, index, result);
}
