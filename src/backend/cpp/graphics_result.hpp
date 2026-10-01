#pragma once

#include "backend/cpp/error_result.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"

namespace tx_generated::graphics
{

inline dynamic_struct option_value(const std::string& type, std::any value = {})
{
    struct_fields fields(2);
    fields[0] = {"present", value.has_value()};
    fields[1] = {"value", std::move(value)};
    return dynamic_struct(dynamic_struct_data{type, "option", std::move(fields)});
}

template<class operation>
int result_call(const char* type, void** result, operation&& run) noexcept
{
    return detail::invoke_leaf([&]
    {
        operation_result<std::any> outcome;
        try
        {
            outcome.value = run();
            outcome.ok = true;
        }
        catch (const runtime_failure& error)
        {
            outcome.error = error.error();
        }
        catch (const std::bad_alloc&)
        {
            outcome.error = {tx::error_kind::graphics, "resource_limit", "图形资源分配失败"};
        }
        *result = detail::make_handle<std::any>(detail::make_result_value(
            type, "error_info", std::move(outcome)));
    }, tx::error_kind::graphics);
}

} // namespace tx_generated::graphics
