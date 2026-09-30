#pragma once

#include "backend/cpp/value_format.hpp"
#include "backend/cpp/vector_value.hpp"
#include "backend/cpp/container_value.hpp"
#include "stdlib/iterator.hpp"
#include "stdlib/closure.hpp"
#include "stdlib/array.hpp"
#include "stdlib/dictionary.hpp"

#include <variant>

namespace tx_generated
{

// 身份表只保存已知的图节点种类，内联持有句柄；实际动态字段仍输出原来的 any。
// 避免每个映射条目又分配一次 any 的间接存储。
using copied_object = std::variant<std::monostate, tx_array, tx_dict, dynamic_struct,
    class_handle, container_handle, tx_iterator, closure_handle, object_vector,
    int_vector, float_vector, bool_vector, string_vector, bytes_vector>;

inline std::any copy_identity_value(const copied_object& value)
{
    return std::visit([](const auto& object) -> std::any
    {
        if constexpr (std::is_same_v<std::decay_t<decltype(object)>, std::monostate>)
        {
            return {};
        }
        else
        {
            return object;
        }
    }, value);
}

} // namespace tx_generated
