#include "frontend/sema/sema.hpp"

#include <algorithm>

namespace tx
{
namespace
{

struct container_signature
{
    value_type result;
    std::vector<value_type> parameters;
};

container_signature method_signature(const value_type& type,
                                     const std::string& name, source_pos position)
{
    const auto& element = type.parameters.front();
    const auto kind = type.container_name();
    if (name == "size")
    {
        return {value_type::int_type, {}};
    }
    if (name == "empty")
    {
        return {value_type::bool_type, {}};
    }
    if (name == "clear")
    {
        return {value_type::void_type, {}};
    }
    if (kind == "deque")
    {
        if (name == "to_vector")
        {
            return {value_type::vector_of(element), {}};
        }
        if (name == "push_front" || name == "push_back")
        {
            return {value_type::void_type, {element}};
        }
        if (name == "pop_front" || name == "pop_back")
        {
            return {value_type::void_type, {}};
        }
        if (name == "front" || name == "back")
        {
            return {element, {}};
        }
        if (name == "insert")
        {
            return {value_type::void_type, {value_type::int_type, element}};
        }
        if (name == "erase")
        {
            return {value_type::void_type, {value_type::int_type}};
        }
    }
    if ((kind == "map" || kind == "set" ||
         kind == "ordered_map" || kind == "ordered_set") &&
        (name == "contains" || name == "remove" ||
         ((kind == "set" || kind == "ordered_set") && name == "insert")))
    {
        return {value_type::bool_type, {element}};
    }
    if ((kind == "map" || kind == "ordered_map") && name == "get")
    {
        return {type.parameters[1], {element, type.parameters[1]}};
    }
    if ((kind == "map" || kind == "ordered_map") &&
        (name == "keys" || name == "values"))
    {
        return {value_type::vector_of(type.parameters[name == "keys" ? 0 : 1]), {}};
    }
    if ((kind == "map" || kind == "ordered_map") && name == "entries")
    {
        return {value_type::vector_of(value_type::container_of(
            "entry", type.parameters)), {}};
    }
    if ((kind == "ordered_map" || kind == "ordered_set") && name == "range")
    {
        const auto result = kind == "ordered_map"
            ? value_type::vector_of(value_type::container_of(
                "entry", type.parameters)) : value_type::vector_of(element);
        return {result, {element, element}};
    }
    if (kind != "map" && kind != "ordered_map" && name == "to_vector")
    {
        return {value_type::vector_of(element), {}};
    }
    if (kind == "heap" || kind == "queue")
    {
        if (name == "push")
        {
            return {value_type::void_type, {element}};
        }
        if (name == "pop")
        {
            return {value_type::void_type, {}};
        }
        if ((kind == "heap" && name == "top") ||
            (kind == "queue" && (name == "front" || name == "back")))
        {
            return {element, {}};
        }
    }
    throw compile_error(position, "未知 " + kind + " 方法：" + name);
}

} // namespace

value_type semantic_analyzer::check_container_call(
    expression& item, call_expression& call, const value_type& type)
{
    validate_type(type, item.position);
    const auto actual = check_call_arguments(call);
    if (type.is_priority_entry())
    {
        if (!call.container_type || actual != std::vector<value_type>{
                value_type::int_type, type.parameters.front()})
        {
            throw compile_error(item.position,
                "priority_entry 构造需要 int 优先级和同类型值");
        }
        return type;
    }
    for (const auto& argument : call.arguments)
    {
        if (argument.kind != argument_kind::positional)
        {
            throw compile_error(argument.position, "类型化容器操作只接受普通位置实参");
        }
    }
    container_signature signature{type, {}};
    if (call.container_type)
    {
        if (type.container_name() == "queue")
        {
            if (!actual.empty())
            {
                signature.parameters = {value_type::vector_of(type.parameters.front())};
            }
        }
        if (type.container_name() == "heap")
        {
            const auto& element = type.parameters.front();
            const auto vector = value_type::vector_of(element);
            const auto compare = value_type::function_of(
                {element, element}, value_type::int_type);
            std::vector<value_type> selected;
            std::size_t index = 0;
            if (index < actual.size() && actual[index] == value_type::bool_type)
            {
                selected.push_back(value_type::bool_type);
                ++index;
            }
            if (index < actual.size() && actual[index] == vector)
            {
                selected.push_back(vector);
                ++index;
            }
            if (index < actual.size() && actual[index] == compare)
            {
                selected.push_back(compare);
                ++index;
            }
            signature.parameters = std::move(selected);
            if (index != actual.size() ||
                (element.is_priority_entry() &&
                 std::find(actual.begin(), actual.end(), compare) != actual.end()))
            {
                throw compile_error(item.position,
                    "heap 构造需要可选的 bool、vector<T>、fn(T,T)->int；"
                    "优先级条目不能另设比较器");
            }
            if (std::find(actual.begin(), actual.end(), compare) == actual.end())
            {
                if (const auto found = structs_.find(element.name);
                    found != structs_.end())
                {
                    bool has_less = false;
                    for (const auto& method : found->second->methods)
                    {
                        has_less |= method.operator_kind == token_kind::less &&
                            method.parameters.size() == 1 &&
                            method.parameters.front().type == element &&
                            method.return_type == value_type::bool_type;
                    }
                    if (!has_less)
                    {
                        throw compile_error(item.position,
                            "heap 复合元素需要 operator < 或显式比较器");
                    }
                }
            }
        }
        if (type.container_name() == "ordered_map" ||
            type.container_name() == "ordered_set")
        {
            const auto& key = type.parameters.front();
            if (!actual.empty())
            {
                signature.parameters = {value_type::function_of(
                    {key, key}, value_type::int_type)};
            }
            else if (const auto found = structs_.find(key.name);
                     found != structs_.end())
            {
                bool has_less = false;
                for (const auto& method : found->second->methods)
                {
                    has_less |= method.operator_kind == token_kind::less &&
                        method.parameters.size() == 1 &&
                        method.parameters.front().type == key &&
                        method.return_type == value_type::bool_type;
                }
                if (!has_less)
                {
                    throw compile_error(item.position,
                        "有序结构体键需要 operator < 或 fn(K,K)->int 比较器");
                }
            }
        }
    }
    else
    {
        signature = method_signature(type, call.name, item.position);
    }
    if (actual != signature.parameters)
    {
        throw compile_error(item.position, type.container_name() +
            " 操作参数数量或类型不匹配：" + call.name);
    }
    return signature.result;
}

} // namespace tx
