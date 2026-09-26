#include "frontend/sema/sema.hpp"

#include <unordered_set>
#include <utility>

namespace tx
{
namespace
{

bool is_builtin_name(const std::string& name)
{
    return name == "print" || name == "len" || name == "array" ||
           name == "dict" || name == "to_float" || name == "is_none" ||
           name == "input" || name == "input_or_none" ||
           name == "any" || name == "void" || name == "unknown" ||
           name == "self" || name == "super" || name == "bind" ||
           name == "assert_send" || name == "assert_sync" ||
           name == "cancel_source" || name == "cancel_token" ||
           name == "encoding_decoder" || name == "encoding_encoder" ||
           name == "regex_pattern";
}

} // namespace

void semantic_analyzer::validate_key_contract(const value_type& type,
                                               source_pos position) const
{
    if (type == value_type::int_type || type == value_type::float_type ||
        type == value_type::bool_type || type == value_type::str_type)
    {
        return;
    }
    const auto found = structs_.find(type.name);
    if (found == structs_.end())
    {
        throw compile_error(position,
            "哈希键必须是内置标量或满足契约的结构体：" + type.name);
    }
    const auto& definition = *found->second;
    bool has_equal = false;
    bool has_hash = false;
    for (const auto& method : definition.methods)
    {
        has_equal |= method.operator_kind == token_kind::equal_equal &&
            method.parameters.size() == 1 &&
            method.parameters.front().type == type &&
            method.return_type == value_type::bool_type;
        has_hash |= method.name == "hash_key" && !method.operator_kind &&
            method.parameters.empty() &&
            method.return_type == value_type::int_type;
    }
    if (!has_equal || !has_hash)
    {
        throw compile_error(position,
            "结构体哈希键必须声明同类型的 operator ==(other: K) -> bool "
            "和 hash_key() -> int");
    }
    for (const auto& field : definition.fields)
    {
        if (field.type == value_type::bytes_type)
        {
            continue;
        }
        // 键只含值字段，避免普通可变别名在插入后改变哈希关系。
        validate_key_contract(field.type, position);
    }
}

void semantic_analyzer::validate_ordered_key_shape(const value_type& type,
                                                    source_pos position) const
{
    if (type == value_type::int_type || type == value_type::float_type ||
        type == value_type::bool_type || type == value_type::str_type ||
        type == value_type::bytes_type)
    {
        return;
    }
    const auto found = structs_.find(type.name);
    if (found == structs_.end())
    {
        throw compile_error(position,
            "有序键必须是内置标量或仅含不可变值字段的结构体：" + type.name);
    }
    for (const auto& field : found->second->fields)
    {
        validate_ordered_key_shape(field.type, position);
    }
}

void semantic_analyzer::validate_type(const value_type& type, source_pos position) const
{
    if (type.is_function())
    {
        for (std::size_t index = 0; index < type.parameters.size(); ++index)
        {
            const auto& part = type.parameters[index];
            if (index + 1 == type.parameters.size() && part == value_type::void_type)
            {
                continue;
            }
            if (part == value_type::void_type)
            {
                throw compile_error(position, "fn 的参数不能是 void");
            }
            validate_type(part, position);
        }
        return;
    }
    if (type.is_entry())
    {
        for (const auto& member : type.parameters)
        {
            if (member == value_type::void_type || member == value_type::none_type ||
                member == value_type::any_type || member.is_function() ||
                member.is_inferred_function())
            {
                throw compile_error(position, "entry 字段必须是可持有的具体非函数类型");
            }
            validate_type(member, position);
        }
        return;
    }
    if (type.is_vector() || type.is_iterator() || type.is_deque() ||
        type.is_priority_entry())
    {
        const auto& element = type.parameters.front();
        if (element == value_type::void_type || element == value_type::none_type ||
            element == value_type::any_type || element.is_function() ||
            element.is_inferred_function())
        {
            throw compile_error(position, type.container_name() +
                " 元素必须是可持有的具体非函数类型");
        }
        validate_type(element, position);
        return;
    }
    if (type.is_sum_type())
    {
        const auto& element = type.parameters.front();
        if (element == value_type::none_type || element.is_function() ||
            element.is_inferred_function() ||
            (type.is_option() && element == value_type::void_type))
        {
            throw compile_error(position, type.container_name() +
                " 类型参数必须是可持有的具体值类型");
        }
        if (element != value_type::void_type)
        {
            validate_type(element, position);
        }
        return;
    }
    if (type.is_typed_container())
    {
        const auto kind = type.container_name();
        if (kind == "map" || kind == "set")
        {
            validate_key_contract(type.parameters.front(), position);
        }
        if (kind == "ordered_map" || kind == "ordered_set")
        {
            if (type.parameters.front() == value_type::bytes_type)
            {
                throw compile_error(position, "bytes 不能直接用作有序键");
            }
            validate_ordered_key_shape(type.parameters.front(), position);
        }
        const auto first_value = kind == "map" || kind == "ordered_map" ? 1U :
            kind == "set" || kind == "ordered_set" ? type.parameters.size() : 0U;
        for (std::size_t index = first_value; index < type.parameters.size(); ++index)
        {
            const auto& element = type.parameters[index];
            if (element == value_type::void_type || element == value_type::none_type ||
                element == value_type::any_type || element.is_function() ||
                element.is_inferred_function())
            {
                throw compile_error(position, kind +
                    " 的值类型必须是可持有的具体非函数类型");
            }
            if (kind == "map" || kind == "ordered_map" ||
                kind == "heap" || kind == "queue")
            {
                validate_type(element, position);
                continue;
            }
            if (element != value_type::int_type && element != value_type::float_type &&
                element != value_type::bool_type && element != value_type::str_type)
            {
                throw compile_error(position, kind +
                    " 的值类型当前支持 int、float、bool、str");
            }
        }
        return;
    }
    if (type == value_type::int_type || type == value_type::bool_type ||
        type == value_type::float_type || type == value_type::str_type ||
        type == value_type::bytes_type ||
        type == value_type::binary_stream_type ||
        type == value_type::text_stream_type ||
        type == value_type::cancel_source_type ||
        type == value_type::cancel_token_type ||
        type == value_type::encoding_decoder_type ||
        type == value_type::encoding_encoder_type ||
        type == value_type::regex_pattern_type ||
        type == value_type::array_type || type == value_type::dict_type ||
        type == value_type::any_type || type == value_type::none_type ||
        structs_.contains(type.name) || classes_.contains(type.name))
    {
        return;
    }
    throw compile_error(position, "未知类型：" + type.name);
}

void semantic_analyzer::register_structs(program& source)
{
    structs_.clear();
    for (const auto& definition : source.structs)
    {
        if (structs_.contains(definition.name) || classes_.contains(definition.name) ||
            is_builtin_name(definition.name))
        {
            throw compile_error(definition.position, "重复或保留的结构体名：" +
                                                      definition.name);
        }
        std::unordered_set<std::string> fields;
        for (const auto& field : definition.fields)
        {
            validate_type(field.type, field.position);
            if (!fields.insert(field.name).second)
            {
                throw compile_error(field.position, "重复字段：" + field.name);
            }
        }
        // 只允许字段引用已经完成声明的结构体，避免生成递归值类型。
        structs_.emplace(definition.name, &definition);
    }
    for (auto& definition : source.structs)
    {
        for (std::size_t index = 0; index < definition.methods.size(); ++index)
        {
            auto& method = definition.methods[index];
            if (method.return_type != value_type::void_type)
            {
                validate_type(method.return_type, method.position);
            }
            std::unordered_set<std::string> names;
            for (const auto& parameter : method.parameters)
            {
                validate_type(parameter.type, parameter.position);
                if (parameter.name == "self" || parameter.name == "super" ||
                    !names.insert(parameter.name).second)
                {
                    throw compile_error(parameter.position,
                                        "重复或保留的运算符参数名：" +
                                        parameter.name);
                }
            }
            if (method.name == "hash_key" && !method.operator_kind)
            {
                if (!method.parameters.empty() ||
                    method.return_type != value_type::int_type)
                {
                    throw compile_error(method.position,
                        "hash_key 必须声明为 def hash_key() -> int");
                }
            }
            else
            {
                validate_operator_method(method);
            }
            std::size_t overload = 0;
            for (std::size_t prior = 0; prior < index; ++prior)
            {
                const auto& previous = definition.methods[prior];
                if (previous.name != method.name)
                {
                    continue;
                }
                if (same_overload_key(previous, method))
                {
                    throw compile_error(method.position,
                                        "重复的运算符重载签名：" +
                                        method.source_name);
                }
                ++overload;
            }
            method.overload_index = overload;
        }
    }
}

void semantic_analyzer::register_functions(const program& source, bool require_main)
{
    functions_.clear();
    algorithm_intrinsics_.clear();
    for (const auto& function : source.functions)
    {
        const auto& display_name = function.source_name.empty()
            ? function.name : function.source_name;
        if ((is_builtin_name(function.name) &&
             function.external_name != "algorithm.any") ||
            structs_.contains(function.name) ||
            classes_.contains(function.name))
        {
            throw compile_error(function.position, "重复或保留的函数名：" + display_name);
        }
        if (function.external && function.name == "main")
        {
            throw compile_error(function.position, ".txh 不能声明 main");
        }
        if (function.return_type != value_type::void_type)
        {
            validate_type(function.return_type, function.position);
        }
        function_signature signature{{}, function.return_type};
        signature.external = function.external;
        signature.position = function.position;
        // 这些标准库边界显式接收 any；其他函数仍使用精确匹配规则。
        signature.accepts_any_value = function.external &&
            (function.external_name == "array.push_back" ||
             function.external_name == "array.insert" ||
             function.external_name == "json.stringify" ||
             function.external_name == "json.stringify_pretty");
        for (const auto& parameter : function.parameters)
        {
            if (!parameter.type.is_inferred_function())
            {
                validate_type(parameter.type, parameter.position);
            }
            else if (function.external)
            {
                throw compile_error(parameter.position,
                                    "外部接口的 fn 参数必须写出完整签名");
            }
            signature.parameters.push_back(parameter);
        }
        auto& overloads = functions_[function.name];
        if (function.external_name.starts_with("algorithm.") &&
            function.external_name != "algorithm.sort" &&
            function.external_name != "algorithm.sorted" &&
            function.external_name != "algorithm.find" &&
            function.external_name != "algorithm.count" &&
            function.external_name != "algorithm.lower_bound" &&
            function.external_name != "algorithm.upper_bound" &&
            function.external_name != "algorithm.reverse" &&
            function.external_name != "algorithm.sum" &&
            function.external_name != "algorithm.min_element" &&
            function.external_name != "algorithm.max_element")
        {
            algorithm_intrinsics_[function.name] =
                function.external_name.substr(10);
        }
        if (function.name == "main" && !overloads.empty())
        {
            throw compile_error(function.position, "main 不能重载");
        }
        for (const auto& existing : overloads)
        {
            bool same_fixed_types = true;
            std::size_t left = 0;
            std::size_t right = 0;
            while (left < existing.parameters.size() &&
                   existing.parameters[left].kind == parameter_kind::ordinary &&
                   right < signature.parameters.size() &&
                   signature.parameters[right].kind == parameter_kind::ordinary)
            {
                if (existing.parameters[left].type != signature.parameters[right].type)
                {
                    same_fixed_types = false;
                    break;
                }
                ++left;
                ++right;
            }
            if (same_fixed_types &&
                (left == existing.parameters.size() ||
                 existing.parameters[left].kind != parameter_kind::ordinary) &&
                (right == signature.parameters.size() ||
                 signature.parameters[right].kind != parameter_kind::ordinary))
            {
                throw compile_error(function.position,
                                    "重复的函数重载签名：" + display_name);
            }
        }
        overloads.push_back(std::move(signature));
    }
    if (!require_main)
    {
        return;
    }
    const auto main = functions_.find("main");
    if (main == functions_.end())
    {
        throw compile_error({1, 1, {}}, "缺少 main 函数");
    }
    const auto& entry = main->second.front();
    if (!entry.parameters.empty() || entry.result != value_type::int_type)
    {
        throw compile_error({1, 1, {}}, "main 必须声明为 def main() -> int");
    }
}

} // namespace tx
