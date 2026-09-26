#include "frontend/sema/sema.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

namespace tx
{
namespace
{

bool is_numeric(const value_type& type)
{
    return type == value_type::int_type || type == value_type::float_type;
}

bool is_equality_type(const value_type& type)
{
    return is_numeric(type) || type == value_type::bool_type ||
           type == value_type::str_type || type == value_type::bytes_type ||
           type == value_type::none_type;
}

bool is_hashable_key_type(const value_type& type)
{
    return (is_equality_type(type) && type != value_type::bytes_type) ||
           type == value_type::any_type;
}

bool is_min_int_magnitude(std::string_view digits)
{
    const auto first = digits.find_first_not_of('0');
    return first != std::string_view::npos &&
           digits.substr(first) == "9223372036854775808";
}

} // namespace

value_type semantic_analyzer::check_literal(expression& item)
{
    if (auto* literal = std::get_if<integer_literal>(&item.data))
    {
        std::int64_t value = 0;
        const auto* end = literal->digits.data() + literal->digits.size();
        const auto [parsed, error] =
            std::from_chars(literal->digits.data(), end, value);
        if (error != std::errc{} || parsed != end)
        {
            throw compile_error(item.position, "整数常量超出 int 范围");
        }
        return value_type::int_type;
    }
    if (auto* literal = std::get_if<floating_literal>(&item.data))
    {
        double value = 0.0;
        const auto* end = literal->digits.data() + literal->digits.size();
        const auto [parsed, error] =
            std::from_chars(literal->digits.data(), end, value);
        if (error != std::errc{} || parsed != end || !std::isfinite(value))
        {
            throw compile_error(item.position, "浮点常量超出 float 范围");
        }
        return value_type::float_type;
    }
    if (std::holds_alternative<string_literal>(item.data))
    {
        return value_type::str_type;
    }
    if (std::holds_alternative<boolean_literal>(item.data))
    {
        return value_type::bool_type;
    }
    if (std::holds_alternative<none_literal>(item.data))
    {
        return value_type::none_type;
    }
    if (auto* literal = std::get_if<array_literal>(&item.data))
    {
        for (auto& element : literal->elements)
        {
            const auto type = check_expression(*element);
            if (type == value_type::void_type)
            {
                throw compile_error(element->position, "数组不能存放无返回值的调用");
            }
        }
        return value_type::array_type;
    }
    auto& literal = std::get<dictionary_literal>(item.data);
    for (auto& entry : literal.entries)
    {
        const auto key_type = check_expression(*entry.key);
        if (!is_hashable_key_type(key_type))
        {
            throw compile_error(entry.key->position, "字典键必须是可哈希的值");
        }
        const auto value = check_expression(*entry.value);
        if (value == value_type::void_type)
        {
            throw compile_error(item.position, "字典不能存放无返回值的调用");
        }
    }
    return value_type::dict_type;
}

value_type semantic_analyzer::check_unary(expression& item,
                                          unary_operation& operation)
{
    if (operation.operation == token_kind::minus)
    {
        const auto* literal = std::get_if<integer_literal>(&operation.operand->data);
        if (literal != nullptr && is_min_int_magnitude(literal->digits))
        {
            // 最小 int 的绝对值超出正数范围，只允许紧随一元负号使用。
            operation.is_min_int_literal = true;
            operation.operand->type = value_type::int_type;
            return value_type::int_type;
        }
    }
    const auto actual = check_expression(*operation.operand);
    if (operation.operation == token_kind::minus && is_numeric(actual))
    {
        return actual;
    }
    if (operation.operation == token_kind::bang &&
        actual == value_type::bool_type)
    {
        return value_type::bool_type;
    }
    if (structs_.contains(actual.name) || classes_.contains(actual.name))
    {
        return bind_operator(operation.operation, actual, std::nullopt,
                             *operation.operand, item.position,
                             operation.binding);
    }
    throw compile_error(item.position, "一元运算符与操作数类型不匹配");
}

value_type semantic_analyzer::check_binary(expression& item,
                                           binary_operation& operation)
{
    const auto left = check_expression(*operation.left);
    const auto right = check_expression(*operation.right);
    switch (operation.operation)
    {
    case token_kind::plus:
        if (left == value_type::str_type && right == value_type::str_type)
        {
            return value_type::str_type;
        }
        [[fallthrough]];
    case token_kind::minus:
    case token_kind::star:
    case token_kind::slash:
        if (left == right && is_numeric(left))
        {
            return left;
        }
        break;
    case token_kind::ampersand:
    case token_kind::caret:
    case token_kind::pipe:
        if (left == value_type::int_type && right == value_type::int_type)
        {
            return value_type::int_type;
        }
        break;
    case token_kind::less:
    case token_kind::less_equal:
    case token_kind::greater:
    case token_kind::greater_equal:
        if (left == right && is_numeric(left))
        {
            return value_type::bool_type;
        }
        break;
    case token_kind::equal_equal:
    case token_kind::bang_equal:
        if (left == right && is_equality_type(left))
        {
            return value_type::bool_type;
        }
        break;
    case token_kind::and_and:
    case token_kind::or_or:
        if (left == value_type::bool_type && right == value_type::bool_type)
        {
            return value_type::bool_type;
        }
        break;
    default:
        break;
    }
    if ((structs_.contains(left.name) || classes_.contains(left.name)) &&
        !operator_method_name(operation.operation).empty())
    {
        return bind_operator(operation.operation, left, right,
                             *operation.left, item.position,
                             operation.binding);
    }
    throw compile_error(item.position, "二元运算符与操作数类型不匹配：" +
                                       std::string(type_name(left)) + " 和 " +
                                       std::string(type_name(right)));
}

value_type semantic_analyzer::check_index(expression&,
                                          index_expression& access)
{
    const auto object_type = check_expression(*access.object);
    if (object_type.is_map())
    {
        require_type(check_expression(*access.index), object_type.parameters[0],
                     access.index->position, "map 键");
        return object_type.parameters[1];
    }
    if (object_type != value_type::array_type &&
        object_type != value_type::dict_type &&
        object_type != value_type::any_type &&
        object_type != value_type::bytes_type && !object_type.is_vector() &&
        !object_type.is_deque())
    {
        throw compile_error(access.object->position, "索引对象需要数组或字典");
    }
    const auto index_type = check_expression(*access.index);
    if (object_type == value_type::array_type ||
        object_type == value_type::bytes_type || object_type.is_vector() ||
        object_type.is_deque())
    {
        require_type(index_type, value_type::int_type,
                     access.index->position, "数组索引");
    }
    else if (object_type == value_type::dict_type &&
             !is_hashable_key_type(index_type))
    {
        throw compile_error(access.index->position, "字典键必须是可哈希的值");
    }
    else if (index_type == value_type::void_type)
    {
        throw compile_error(access.index->position, "字典键不能是 void");
    }
    return object_type == value_type::bytes_type ? value_type::int_type
        : object_type.is_vector() || object_type.is_deque() ?
            object_type.parameters.front()
        : value_type::any_type;
}

value_type semantic_analyzer::check_member(expression& item,
                                           member_expression& access)
{
    const auto object_type = check_expression(*access.object);
    if (object_type == value_type::any_type)
    {
        return value_type::any_type;
    }
    if (object_type.is_priority_entry())
    {
        if (access.field == "priority")
        {
            return value_type::int_type;
        }
        if (access.field == "value")
        {
            return object_type.parameters[0];
        }
        throw compile_error(item.position,
            "未知 priority_entry 字段：" + access.field);
    }
    if (object_type.is_entry())
    {
        if (access.field == "key")
        {
            return object_type.parameters[0];
        }
        if (access.field == "value")
        {
            return object_type.parameters[1];
        }
        throw compile_error(item.position, "未知 entry 字段：" + access.field);
    }
    if (const auto found_class = classes_.find(object_type.name);
        found_class != classes_.end())
    {
        const auto fields = collect_fields(*found_class->second, access.field);
        if (fields.empty())
        {
            throw compile_error(item.position, "未知类字段：" + access.field);
        }
        if (fields.size() != 1)
        {
            throw compile_error(item.position,
                                "多继承字段存在歧义，请转换到指定父类型：" +
                                access.field);
        }
        check_access(fields.front().field->access, *fields.front().owner,
                     item.position, access.field);
        access.field_slot = fields.front().field->slot;
        return fields.front().field->type;
    }
    const auto found = structs_.find(object_type.name);
    if (found == structs_.end())
    {
        throw compile_error(item.position, "字段访问需要结构体类型");
    }
    for (const auto& field : found->second->fields)
    {
        if (field.name == access.field)
        {
            return field.type;
        }
    }
    throw compile_error(item.position, "未知字段：" + access.field);
}

value_type semantic_analyzer::check_cast(expression& item, cast_expression& cast)
{
    if (cast.target == value_type::bytes_type ||
        cast.target == value_type::binary_stream_type ||
        cast.target == value_type::text_stream_type ||
        cast.target == value_type::cancel_source_type ||
        cast.target == value_type::cancel_token_type)
    {
        const auto source = check_expression(*cast.value);
        if (source != cast.target && source != value_type::any_type)
        {
            throw compile_error(item.position, "资源或字节转换需要相同类型或 any");
        }
        return cast.target;
    }
    if (cast.target.is_function() || cast.target.is_vector() ||
        cast.target.is_iterator() ||
        cast.target.is_entry() || cast.target.is_priority_entry() ||
        cast.target.is_typed_container() ||
        cast.target.is_sum_type() || structs_.contains(cast.target.name))
    {
        validate_type(cast.target, item.position);
        const auto source = check_expression(*cast.value);
        if (source != cast.target && source != value_type::any_type)
        {
            throw compile_error(item.position, "类型化容器转换需要相同类型或 any");
        }
        return cast.target;
    }
    if (classes_.contains(cast.target.name))
    {
        const auto actual = check_expression(*cast.value);
        if (actual != value_type::any_type && !classes_.contains(actual.name))
        {
            throw compile_error(item.position,
                                "类或接口转换需要对象引用或 any");
        }
        return cast.target;
    }
    if (cast.target == value_type::bool_type)
    {
        const auto actual = check_expression(*cast.value);
        if (actual != value_type::bool_type &&
            actual != value_type::any_type)
        {
            throw compile_error(item.position,
                                "bool 转换需要 bool 或 any");
        }
        return cast.target;
    }
    const auto numeric_target = is_numeric(cast.target);
    if (!numeric_target && cast.target != value_type::str_type)
    {
        throw compile_error(item.position, "as 目前只支持 int、float 和 str 转换");
    }
    const auto actual = check_expression(*cast.value);
    const auto numeric_source = is_numeric(actual) ||
                                actual == value_type::str_type ||
                                actual == value_type::any_type;
    const auto string_source = numeric_source || actual == value_type::bool_type;
    if ((numeric_target && !numeric_source) ||
        (!numeric_target && !string_source))
    {
        throw compile_error(item.position, "as 的源类型与目标类型不兼容");
    }
    return cast.target;
}

value_type semantic_analyzer::check_expression(expression& item)
{
    if (std::holds_alternative<integer_literal>(item.data) ||
        std::holds_alternative<floating_literal>(item.data) ||
        std::holds_alternative<string_literal>(item.data) ||
        std::holds_alternative<boolean_literal>(item.data) ||
        std::holds_alternative<none_literal>(item.data) ||
        std::holds_alternative<array_literal>(item.data) ||
        std::holds_alternative<dictionary_literal>(item.data))
    {
        item.type = check_literal(item);
    }
    else if (auto* name = std::get_if<name_reference>(&item.data))
    {
        const auto* symbol = find_symbol(name->name);
        if (symbol != nullptr)
        {
            item.type = symbol->type;
        }
        else if (name->ambiguous_function)
        {
            throw compile_error(item.position, "导入函数存在歧义，请使用 as 别名：" +
                                                name->name);
        }
        else if (!name->function_symbol.empty())
        {
            const auto found = functions_.find(name->function_symbol);
            if (found == functions_.end() || found->second.size() != 1 ||
                found->second.front().external)
            {
                throw compile_error(item.position,
                                    "只能引用未重载的普通 TX 函数：" + name->name);
            }
            const auto& signature = found->second.front();
            std::vector<value_type> arguments;
            for (const auto& parameter : signature.parameters)
            {
                if (parameter.kind != parameter_kind::ordinary)
                {
                    throw compile_error(item.position,
                                        "可变参数函数暂不能作为函数值：" + name->name);
                }
                arguments.push_back(parameter.type);
            }
            name->function_value = true;
            item.type = value_type::function_of(std::move(arguments),
                                                signature.result);
        }
        else
        {
            throw compile_error(item.position, "未定义变量：" + name->name);
        }
    }
    else if (auto* access = std::get_if<index_expression>(&item.data))
    {
        item.type = check_index(item, *access);
    }
    else if (auto* access = std::get_if<member_expression>(&item.data))
    {
        item.type = check_member(item, *access);
    }
    else if (auto* cast = std::get_if<cast_expression>(&item.data))
    {
        item.type = check_cast(item, *cast);
    }
    else if (auto* operation = std::get_if<unary_operation>(&item.data))
    {
        item.type = check_unary(item, *operation);
    }
    else if (auto* operation = std::get_if<update_expression>(&item.data))
    {
        item.type = check_lvalue(*operation->target);
        if (!is_numeric(item.type))
        {
            throw compile_error(item.position, "++ 和 -- 只支持 int 或 float");
        }
    }
    else if (auto* operation = std::get_if<binary_operation>(&item.data))
    {
        item.type = check_binary(item, *operation);
    }
    else if (auto* call = std::get_if<call_expression>(&item.data))
    {
        item.type = check_call(item, *call);
    }
    return item.type;
}

} // namespace tx
