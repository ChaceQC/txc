#pragma once

#include "stdlib/vector.hpp"

#include <any>
#include <string_view>

namespace tx_generated
{

// 仅用于 any、print、deep_copy 等动态边界，原生元素访问不经过此分派。
template<class operation>
bool visit_vector(const std::any& value, operation&& apply)
{
    if (const auto* vector = std::any_cast<int_vector>(&value))
    {
        apply(*vector);
    }
    else if (const auto* vector = std::any_cast<float_vector>(&value))
    {
        apply(*vector);
    }
    else if (const auto* vector = std::any_cast<bool_vector>(&value))
    {
        apply(*vector);
    }
    else if (const auto* vector = std::any_cast<string_vector>(&value))
    {
        apply(*vector);
    }
    else if (const auto* vector = std::any_cast<bytes_vector>(&value))
    {
        apply(*vector);
    }
    else if (const auto* vector = std::any_cast<object_vector>(&value))
    {
        apply(*vector);
    }
    else
    {
        return false;
    }
    return true;
}

inline bool vector_matches(const std::any& value, std::string_view name)
{
    return (name == "vector<int>" && value.type() == typeid(int_vector)) ||
           (name == "vector<float>" && value.type() == typeid(float_vector)) ||
           (name == "vector<bool>" && value.type() == typeid(bool_vector)) ||
           (name == "vector<str>" && value.type() == typeid(string_vector)) ||
           (name == "vector<bytes>" && value.type() == typeid(bytes_vector)) ||
           (value.type() == typeid(object_vector) &&
            name.starts_with("vector<") && name.ends_with(">") &&
            std::any_cast<const object_vector&>(value).data().type_name ==
                name.substr(7, name.size() - 8));
}

} // namespace tx_generated
