#include "backend/llvm/codegen.hpp"

#include <algorithm>
#include <utility>

namespace tx
{

llvm_code_generator::ir_value llvm_code_generator::emit_variadic_array(
    const std::vector<ir_value>& values, source_pos position)
{
    const auto slot = allocate(value_type::array_type, position);
    const auto created = temporary();
    write_instruction(created + " = call i32 @txrt_array_new(i64 0, ptr " +
                      slot + ")");
    write_instruction("call void @txrt_require_success(i32 " + created + ")");
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + slot);
    for (const auto& value : values)
    {
        const auto boxed = box_any(value, position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_array_append(ptr " +
                          result + ", ptr " + boxed.text + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        release(boxed);
        release(value);
    }
    return {value_type::array_type, result};
}

llvm_code_generator::ir_value llvm_code_generator::emit_variadic_dict(
    const std::vector<std::pair<std::string, ir_value>>& values,
    source_pos position)
{
    const auto slot = allocate(value_type::dict_type, position);
    const auto created = temporary();
    write_instruction(created + " = call i32 @txrt_dict_new(ptr " + slot + ")");
    write_instruction("call void @txrt_require_success(i32 " + created + ")");
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + slot);
    for (const auto& [name, value] : values)
    {
        const auto boxed = box_any(value, position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_keyword_set(ptr " +
                          result + ", ptr " + global_bytes(name) + ", ptr " +
                          boxed.text + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        release(boxed);
        release(value);
    }
    return {value_type::dict_type, result};
}

std::vector<llvm_code_generator::ir_value>
llvm_code_generator::emit_static_arguments(
    const expression& item, const call_expression& call,
    const std::vector<parameter>& parameters)
{
    const auto fixed_count = static_cast<std::size_t>(std::count_if(
        parameters.begin(), parameters.end(), [](const parameter& entry)
        { return entry.kind == parameter_kind::ordinary; }));
    std::vector<ir_value> result(parameters.size(),
                                 {value_type::void_type, {}});
    std::vector<ir_value> extra_positional;
    std::vector<std::pair<std::string, ir_value>> extra_keywords;
    std::size_t positional = 0;
    // 先按源码顺序求值，再把已知实参放到目标形参位置。
    for (const auto& argument : call.arguments)
    {
        const auto value = expression_value(*argument.value);
        if (argument.kind == argument_kind::positional)
        {
            if (positional < fixed_count)
            {
                result[positional] = value;
            }
            else
            {
                extra_positional.push_back(value);
            }
            ++positional;
            continue;
        }
        const auto found = std::find_if(parameters.begin(),
            parameters.begin() + fixed_count,
            [&](const parameter& entry) { return entry.name == argument.name; });
        if (found == parameters.begin() + fixed_count)
        {
            extra_keywords.emplace_back(argument.name, value);
        }
        else
        {
            result[static_cast<std::size_t>(found - parameters.begin())] = value;
        }
    }
    for (std::size_t index = fixed_count; index < parameters.size(); ++index)
    {
        result[index] = parameters[index].kind == parameter_kind::variadic_array
            ? emit_variadic_array(extra_positional, item.position)
            : emit_variadic_dict(extra_keywords, item.position);
    }
    return result;
}

std::optional<std::vector<llvm_code_generator::ir_value>>
llvm_code_generator::emit_direct_spreads(
    const expression& item, const call_expression& call,
    const std::vector<parameter>& parameters)
{
    if (parameters.size() < 2 || call.arguments.size() != parameters.size())
    {
        return std::nullopt;
    }
    const auto fixed_count = parameters.size() - 2;
    if (parameters[fixed_count].kind != parameter_kind::variadic_array ||
        parameters[fixed_count + 1].kind != parameter_kind::variadic_dict ||
        call.arguments[fixed_count].kind != argument_kind::spread_array ||
        call.arguments[fixed_count + 1].kind != argument_kind::spread_dict)
    {
        return std::nullopt;
    }
    for (std::size_t index = 0; index < fixed_count; ++index)
    {
        if (parameters[index].kind != parameter_kind::ordinary ||
            call.arguments[index].kind != argument_kind::positional)
        {
            return std::nullopt;
        }
    }

    std::vector<ir_value> result;
    result.reserve(parameters.size());
    for (std::size_t index = 0; index < fixed_count; ++index)
    {
        result.push_back(expression_value(*call.arguments[index].value));
    }
    const auto spread_array = expression_value(
        *call.arguments[fixed_count].value);
    if (spread_array.type == value_type::any_type)
    {
        // 动态值先验证 * 类型，再求值后续 ** 实参。
        write_instruction("call void @txrt_array_require_spread(ptr " +
                          spread_array.text + ")");
    }
    const auto spread_dict = expression_value(
        *call.arguments[fixed_count + 1].value);
    std::string names = "null";
    if (fixed_count != 0)
    {
        names = "%slot" + std::to_string(next_slot_++);
        allocations_ << "  " << names << " = alloca ptr, i64 "
                     << fixed_count << '\n';
        for (std::size_t index = 0; index < fixed_count; ++index)
        {
            const auto address = temporary();
            write_instruction(address + " = getelementptr ptr, ptr " + names +
                              ", i64 " + std::to_string(index));
            write_instruction("store ptr " + global_bytes(parameters[index].name) +
                              ", ptr " + address);
        }
    }
    const auto args_slot = allocate(value_type::array_type, item.position);
    const auto kwargs_slot = allocate(value_type::dict_type, item.position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_call_split_spreads(ptr " +
        spread_array.text + ", ptr " + spread_dict.text + ", ptr " +
        names + ", i64 " + std::to_string(fixed_count) + ", ptr " +
        args_slot + ", ptr " + kwargs_slot + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    release(spread_array);
    release(spread_dict);
    const auto args = temporary();
    write_instruction(args + " = load ptr, ptr " + args_slot);
    result.push_back({value_type::array_type, args});
    const auto kwargs = temporary();
    write_instruction(kwargs + " = load ptr, ptr " + kwargs_slot);
    result.push_back({value_type::dict_type, kwargs});
    return result;
}

std::vector<llvm_code_generator::ir_value>
llvm_code_generator::emit_bound_arguments(
    const expression& item, const call_expression& call,
    const std::vector<parameter>& parameters)
{
    const bool has_spread = std::any_of(call.arguments.begin(),
        call.arguments.end(), [](const call_argument& argument)
        {
            return argument.kind == argument_kind::spread_array ||
                   argument.kind == argument_kind::spread_dict;
        });
    if (!has_spread)
    {
        return emit_static_arguments(item, call, parameters);
    }
    if (auto direct = emit_direct_spreads(item, call, parameters))
    {
        return std::move(*direct);
    }
    const auto positional_slot = allocate(value_type::array_type, item.position);
    const auto positional_status = temporary();
    write_instruction(positional_status + " = call i32 @txrt_array_new(i64 0, ptr " +
                      positional_slot + ")");
    write_instruction("call void @txrt_require_success(i32 " + positional_status + ")");
    const auto positional = temporary();
    write_instruction(positional + " = load ptr, ptr " + positional_slot);

    const auto keyword_slot = allocate(value_type::dict_type, item.position);
    const auto keyword_status = temporary();
    write_instruction(keyword_status + " = call i32 @txrt_dict_new(ptr " +
                      keyword_slot + ")");
    write_instruction("call void @txrt_require_success(i32 " + keyword_status + ")");
    const auto keywords = temporary();
    write_instruction(keywords + " = load ptr, ptr " + keyword_slot);

    for (const auto& argument : call.arguments)
    {
        const auto value = expression_value(*argument.value);
        std::string invocation;
        ir_value boxed{value_type::void_type, {}};
        if (argument.kind == argument_kind::positional ||
            argument.kind == argument_kind::keyword)
        {
            boxed = box_any(value, argument.position);
        }
        if (argument.kind == argument_kind::positional)
        {
            invocation = "@txrt_array_append(ptr " + positional +
                         ", ptr " + boxed.text;
        }
        else if (argument.kind == argument_kind::spread_array)
        {
            invocation = "@txrt_array_extend(ptr " + positional +
                         ", ptr " + value.text;
        }
        else if (argument.kind == argument_kind::keyword)
        {
            invocation = "@txrt_keyword_set(ptr " + keywords + ", ptr " +
                         global_bytes(argument.name) + ", ptr " + boxed.text;
        }
        else
        {
            invocation = "@txrt_keyword_merge(ptr " + keywords +
                         ", ptr " + value.text;
        }
        const auto status = temporary();
        write_instruction(status + " = call i32 " + invocation + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        release(boxed);
        release(value);
    }

    std::size_t fixed_count = 0;
    bool accepts_args = false;
    bool accepts_kwargs = false;
    for (const auto& parameter : parameters)
    {
        if (parameter.kind == parameter_kind::ordinary)
        {
            ++fixed_count;
        }
        else if (parameter.kind == parameter_kind::variadic_array)
        {
            accepts_args = true;
        }
        else
        {
            accepts_kwargs = true;
        }
    }
    std::string names = "null";
    if (fixed_count != 0)
    {
        names = "%slot" + std::to_string(next_slot_++);
        allocations_ << "  " << names << " = alloca ptr, i64 " << fixed_count << '\n';
        for (std::size_t index = 0; index < fixed_count; ++index)
        {
            const auto address = temporary();
            write_instruction(address + " = getelementptr ptr, ptr " + names +
                              ", i64 " + std::to_string(index));
            write_instruction("store ptr " + global_bytes(parameters[index].name) +
                              ", ptr " + address);
        }
    }
    const auto bound_slot = allocate(value_type::array_type, item.position);
    const auto bind_status = temporary();
    write_instruction(bind_status + " = call i32 @txrt_call_bind(ptr " +
                      positional + ", ptr " + keywords + ", ptr " + names +
                      ", i64 " + std::to_string(fixed_count) + ", i32 " +
                      std::to_string(accepts_args) + ", i32 " +
                      std::to_string(accepts_kwargs) + ", ptr " + bound_slot + ")");
    write_instruction("call void @txrt_require_success(i32 " + bind_status + ")");
    release({value_type::array_type, positional});
    release({value_type::dict_type, keywords});
    const auto bound = temporary();
    write_instruction(bound + " = load ptr, ptr " + bound_slot);

    std::vector<ir_value> result;
    result.reserve(parameters.size());
    for (std::size_t index = 0; index < parameters.size(); ++index)
    {
        const auto address = allocate(value_type::any_type, item.position, false);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_array_element_address(ptr " +
                          bound + ", i64 " + std::to_string(index) +
                          ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        const auto borrowed = temporary();
        write_instruction(borrowed + " = load ptr, ptr " + address);
        const auto type_status = temporary();
        write_instruction(type_status + " = call i32 @txrt_value_require_type(ptr " +
                          borrowed + ", ptr " +
                          global_bytes(parameters[index].type.name) + ")");
        write_instruction("call void @txrt_require_success(i32 " + type_status + ")");
        result.push_back(from_any({value_type::any_type, borrowed},
                                  parameters[index].type, item.position));
    }
    release({value_type::array_type, bound});
    return result;
}

} // namespace tx
