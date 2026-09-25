#include "backend/llvm/codegen.hpp"

namespace tx
{

llvm_code_generator::variable_slot llvm_code_generator::make_scalar_snapshot(
    const std::string& element, source_pos position)
{
    const auto fallback = allocate(value_type::any_type, position);
    const auto bits = allocate(value_type::int_type, position);
    const auto kind = "%slot" + std::to_string(next_slot_++);
    allocations_ << "  " << kind << " = alloca i8\n";
    write_instruction("store ptr null, ptr " + fallback);
    const auto scalar = temporary();
    write_instruction(scalar +
        " = call i1 @txrt_value_snapshot_scalar(ptr " + element +
        ", ptr " + kind + ", ptr " + bits + ")");
    const auto clone_label = label();
    const auto ready_label = label();
    write_instruction("br i1 " + scalar + ", label %" + ready_label +
                      ", label %" + clone_label);
    start_block(clone_label);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_value_clone(ptr " +
                      element + ", ptr " + fallback + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    write_instruction("br label %" + ready_label);
    start_block(ready_label);
    return {value_type::any_type, fallback, false, {}, kind, bits};
}

llvm_code_generator::ir_value llvm_code_generator::cast_scalar_snapshot(
    const variable_slot& variable, const value_type& target,
    source_pos position)
{
    const auto kind = temporary();
    write_instruction(kind + " = load i8, ptr " + variable.snapshot_kind);
    const auto bits = temporary();
    write_instruction(bits + " = load i64, ptr " + variable.snapshot_bits);
    const auto fallback = temporary();
    write_instruction(fallback + " = load ptr, ptr " + variable.address);
    const auto expected = target == value_type::int_type ? 2
        : target == value_type::float_type ? 3 : 4;
    const auto direct = temporary();
    write_instruction(direct + " = icmp eq i8 " + kind + ", " +
                      std::to_string(expected));
    const auto direct_label = label();
    const auto convert_label = label();
    const auto ready_label = label();
    const auto output = allocate(target, position);
    write_instruction("br i1 " + direct + ", label %" + direct_label +
                      ", label %" + convert_label);
    start_block(direct_label);
    std::string scalar = bits;
    if (target == value_type::float_type)
    {
        scalar = temporary();
        write_instruction(scalar + " = bitcast i64 " + bits + " to double");
    }
    else if (target == value_type::bool_type)
    {
        scalar = temporary();
        write_instruction(scalar + " = trunc i64 " + bits + " to i1");
    }
    write_instruction("store " + llvm_type(target, position) + " " + scalar +
                      ", ptr " + output);
    write_instruction("br label %" + ready_label);
    start_block(convert_label);
    const auto converted = temporary();
    const auto* function = target == value_type::int_type
        ? "txrt_value_snapshot_to_i64_fast"
        : target == value_type::float_type
        ? "txrt_value_snapshot_to_f64_fast"
        : "txrt_value_snapshot_to_bool_fast";
    write_instruction(converted + " = call " + llvm_type(target, position) +
                      " @" + function + "(i8 " + kind + ", i64 " + bits +
                      ", ptr " + fallback + ")");
    write_instruction("store " + llvm_type(target, position) + " " +
                      converted + ", ptr " + output);
    write_instruction("br label %" + ready_label);
    start_block(ready_label);
    return load({target, output});
}

} // namespace tx
