#pragma once

#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/typed_container.hpp"
#include "stdlib/vector.hpp"

#include <limits>
#include <stdexcept>

namespace tx_generated::detail
{

template<class storage_type, class... arguments>
int container_new(void** result, arguments... values) noexcept
{
    return invoke_checked([&]
    {
        container_handle container = std::make_shared<storage_type>(values...);
        *result = make_handle<std::any>(std::move(container));
        note_gc_allocation();
    });
}

template<class storage_type, class operation>
int container_apply(const void* value, operation&& apply) noexcept
{
    constexpr auto effect = storage_type::has_user_effects
        ? error_effect::may_run_user_code : error_effect::local_only;
    return invoke_checked<effect>([&]
    {
        // 调用符号已由编译器按完整类型选定；any 转换在进入此路径之前检查。
        const auto& container = std::any_cast<const container_handle&>(
            *static_cast<const std::any*>(value));
        apply(static_cast<storage_type&>(*container));
    });
}

template<class value_type, class output_type>
void container_result(const value_type& value, output_type* result)
{
    *result = static_cast<output_type>(value);
}

inline void container_result(std::size_t value, std::int64_t* result)
{
    if (value > static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()))
    {
        throw std::overflow_error("容器长度超出 int 范围");
    }
    *result = static_cast<std::int64_t>(value);
}

inline void container_result(const text_reference& value, void** result)
{
    *result = copy_text_handle(value.handle());
}

template<class element_type>
void container_result(tx_vector<element_type> value, void** result)
{
    *result = make_handle<std::any>(std::move(value));
}

} // namespace tx_generated::detail
