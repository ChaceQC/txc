#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

std::vector<llvm_code_generator::ir_value>
llvm_code_generator::emit_bound_arguments(
    const expression& item, const call_expression& call,
    const std::vector<parameter>& parameters)
{
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
        const auto address = allocate(value_type::any_type, item.position);
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
