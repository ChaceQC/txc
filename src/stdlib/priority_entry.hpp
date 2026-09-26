#pragma once

#include "stdlib/map_entry.hpp"

#include <any>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

namespace tx_generated
{

template<class value_type>
[[nodiscard]] dynamic_struct make_priority_entry(
    const char* type_name, std::int64_t priority, const value_type& value)
{
    if (!type_name)
    {
        throw std::runtime_error("priority_entry 缺少静态类型");
    }
    struct_fields fields(2);
    fields[0] = {"priority", priority};
    fields[1] = {"value", entry_field_value(value)};
    return dynamic_struct(dynamic_struct_data{
        type_name, "priority_entry", std::move(fields)});
}

[[nodiscard]] inline std::int64_t priority_of(const std::any& value)
{
    const auto* item = std::any_cast<dynamic_struct>(&value);
    if (!item || !(*item)->type_name.starts_with("priority_entry<") ||
        (*item)->fields.size() != 2)
    {
        throw std::runtime_error("heap 优先级条目类型不匹配");
    }
    return std::any_cast<std::int64_t>((*item)->fields[0].value);
}

} // namespace tx_generated
