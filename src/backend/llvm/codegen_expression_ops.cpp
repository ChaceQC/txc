#include "backend/llvm/codegen.hpp"

namespace tx
{

llvm_code_generator::ir_value llvm_code_generator::checked_binary(
    const std::string& name, const ir_value& left, const ir_value& right,
    const value_type& result_type, source_pos position)
{
    const char* intrinsic = nullptr;
    if (result_type == value_type::int_type)
    {
        intrinsic = name == "txrt_add_i64" ? "llvm.sadd.with.overflow.i64"
            : name == "txrt_sub_i64" ? "llvm.ssub.with.overflow.i64"
            : name == "txrt_mul_i64" ? "llvm.smul.with.overflow.i64"
            : nullptr;
    }
    if (intrinsic)
    {
        // 正常路径只执行 LLVM 整数运算；溢出时沿用运行时原有的中文错误。
        const auto checked = temporary();
        write_instruction(checked + " = call { i64, i1 } @" + intrinsic +
                          "(i64 " + left.text + ", i64 " + right.text + ")");
        const auto result = temporary();
        write_instruction(result + " = extractvalue { i64, i1 } " + checked + ", 0");
        const auto overflow = temporary();
        write_instruction(overflow + " = extractvalue { i64, i1 } " + checked + ", 1");
        const auto error_label = label();
        const auto success_label = label();
        write_instruction("br i1 " + overflow + ", label %" + error_label +
                          ", label %" + success_label);
        start_block(error_label);
        const auto address = allocate(result_type, position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @" + name + "(i64 " +
                          left.text + ", i64 " + right.text + ", ptr " +
                          address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        write_instruction("unreachable");
        start_block(success_label);
        return {result_type, result};
    }

    const auto type = llvm_type(result_type, position);
    const auto address = allocate(result_type, position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + name + "(" + type + " " +
                      left.text + ", " + type + " " + right.text +
                      ", ptr " + address + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    if (result_type == value_type::str_type)
    {
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + address);
        return {result_type, result};
    }
    return load({result_type, address});
}

llvm_code_generator::ir_value llvm_code_generator::checked_unary(
    const std::string& name, const ir_value& value,
    const value_type& result_type, source_pos position)
{
    if (result_type == value_type::int_type && name == "txrt_neg_i64")
    {
        const auto checked = temporary();
        write_instruction(checked + " = call { i64, i1 } @llvm.ssub.with.overflow.i64"
                          "(i64 0, i64 " + value.text + ")");
        const auto result = temporary();
        write_instruction(result + " = extractvalue { i64, i1 } " + checked + ", 0");
        const auto overflow = temporary();
        write_instruction(overflow + " = extractvalue { i64, i1 } " + checked + ", 1");
        const auto error_label = label();
        const auto success_label = label();
        write_instruction("br i1 " + overflow + ", label %" + error_label +
                          ", label %" + success_label);
        start_block(error_label);
        const auto address = allocate(result_type, position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_neg_i64(i64 " +
                          value.text + ", ptr " + address + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        write_instruction("unreachable");
        start_block(success_label);
        return {result_type, result};
    }

    const auto type = llvm_type(result_type, position);
    const auto address = allocate(result_type, position);
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + name + "(" + type + " " +
                      value.text + ", ptr " + address + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    return load({result_type, address});
}

llvm_code_generator::ir_value llvm_code_generator::short_circuit(
    const binary_operation& operation)
{
    const auto left = expression_value(*operation.left);
    const auto address = allocate(value_type::bool_type,
                                  operation.left->position);
    write_instruction("store i1 " + left.text + ", ptr " + address);
    const auto right_label = label();
    const auto end_label = label();
    if (operation.operation == token_kind::and_and)
    {
        write_instruction("br i1 " + left.text + ", label %" + right_label +
                          ", label %" + end_label);
    }
    else
    {
        write_instruction("br i1 " + left.text + ", label %" + end_label +
                          ", label %" + right_label);
    }
    start_block(right_label);
    const auto right = expression_value(*operation.right);
    write_instruction("store i1 " + right.text + ", ptr " + address);
    write_instruction("br label %" + end_label);
    start_block(end_label);
    return load({value_type::bool_type, address});
}

llvm_code_generator::ir_value llvm_code_generator::emit_operator_call(
    const value_type& result_type, source_pos position,
    const operator_binding& binding, const ir_value& receiver,
    const std::optional<ir_value>& argument,
    bool receiver_borrowed, bool argument_borrowed)
{
    std::string callee = function_name(binding.symbol, binding.overload_index);
    if (binding.virtual_slot)
    {
        callee = temporary();
        write_instruction(callee +
            " = call ptr @txrt_class_virtual_target_fast(ptr " +
            receiver.text + ", i64 " +
            std::to_string(*binding.virtual_slot) + ")");
    }
    std::string arguments = "ptr " + receiver.text;
    if (argument)
    {
        arguments += ", " + llvm_type(argument->type, position) +
                     " " + argument->text;
    }
    const auto result = temporary();
    if (argument && !operator_argument_borrowed(binding))
    {
        forget_owned_value(*argument);
    }
    write_instruction(result + " = call " + llvm_type(result_type, position) +
                      " " + callee + "(" + arguments + ")");
    const auto owned_result = own_direct_value({result_type, result});
    if (!receiver_borrowed)
    {
        release(receiver);
    }
    if (argument && operator_argument_borrowed(binding) &&
        !argument_borrowed)
    {
        release(*argument);
    }
    return owned_result;
}

llvm_code_generator::ir_value llvm_code_generator::emit_binary(
    const expression& item, const binary_operation& operation)
{
    if (operation.operation == token_kind::and_and ||
        operation.operation == token_kind::or_or)
    {
        return short_circuit(operation);
    }
    if (auto literal = emit_literal_string_compare(operation))
    {
        return *literal;
    }
    if (auto concatenated = emit_string_concat(item, operation))
    {
        return *concatenated;
    }
    bool left_borrowed = false;
    bool right_borrowed = false;
    const auto left = operation.binding
        ? expression_value_or_borrow(*operation.left, left_borrowed)
        : expression_value(*operation.left);
    const auto right = operation.binding &&
        operator_argument_borrowed(*operation.binding)
        ? expression_value_or_borrow(*operation.right, right_borrowed)
        : expression_value(*operation.right);
    if (operation.binding)
    {
        return emit_operator_call(item.type, item.position, *operation.binding,
                                  left, right, left_borrowed, right_borrowed);
    }
    const auto& type = operation.left->type;
    const auto kind = operation.operation;

    if (type == value_type::none_type)
    {
        release(left);
        release(right);
        return {item.type, kind == token_kind::equal_equal ? "true" : "false"};
    }

    if (type == value_type::str_type)
    {
        if (kind == token_kind::plus)
        {
            const auto result = checked_binary("txrt_str_concat", left, right,
                                               type, item.position);
            release(left);
            release(right);
            return result;
        }
        if (kind == token_kind::equal_equal || kind == token_kind::bang_equal)
        {
            const auto address = "%slot" + std::to_string(next_slot_++);
            allocations_ << "  " << address << " = alloca i32\n";
            const auto status = temporary();
            write_instruction(status + " = call i32 @txrt_str_compare(ptr " +
                              left.text + ", ptr " + right.text + ", ptr " +
                              address + ")");
            write_instruction("call void @txrt_require_success(i32 " + status + ")");
            release(left);
            release(right);
            const auto compared = temporary();
            write_instruction(compared + " = load i32, ptr " + address);
            const auto result = temporary();
            write_instruction(result + " = icmp " +
                std::string(kind == token_kind::equal_equal ? "eq" : "ne") +
                " i32 " + compared + ", 0");
            return {item.type, result};
        }
        throw compile_error(item.position, "LLVM 后端暂不支持此字符串运算");
    }

    if (type == value_type::int_type)
    {
        if (kind == token_kind::plus || kind == token_kind::minus ||
            kind == token_kind::star || kind == token_kind::slash)
        {
            const auto* name = kind == token_kind::plus ? "txrt_add_i64"
                : kind == token_kind::minus ? "txrt_sub_i64"
                : kind == token_kind::star ? "txrt_mul_i64" : "txrt_div_i64";
            return checked_binary(name, left, right, type, item.position);
        }
    }
    else if (type == value_type::float_type)
    {
        if (kind == token_kind::slash)
        {
            return checked_binary("txrt_div_f64", left, right,
                                  type, item.position);
        }
        const auto* arithmetic = kind == token_kind::plus ? "fadd"
            : kind == token_kind::minus ? "fsub"
            : kind == token_kind::star ? "fmul" : nullptr;
        if (arithmetic)
        {
            const auto result = temporary();
            write_instruction(result + " = " + arithmetic + " double " +
                              left.text + ", " + right.text);
            return {item.type, result};
        }
    }

    const auto* comparison = kind == token_kind::equal_equal ? "eq"
        : kind == token_kind::bang_equal ? "ne"
        : kind == token_kind::less ? "slt"
        : kind == token_kind::less_equal ? "sle"
        : kind == token_kind::greater ? "sgt"
        : kind == token_kind::greater_equal ? "sge" : nullptr;
    if (!comparison)
    {
        throw compile_error(item.position, "LLVM 后端暂不支持此二元运算");
    }
    const auto result = temporary();
    if (type == value_type::float_type)
    {
        const auto* predicate = kind == token_kind::equal_equal ? "oeq"
            : kind == token_kind::bang_equal ? "une"
            : kind == token_kind::less ? "olt"
            : kind == token_kind::less_equal ? "ole"
            : kind == token_kind::greater ? "ogt" : "oge";
        write_instruction(result + " = fcmp " + predicate + " double " +
                          left.text + ", " + right.text);
    }
    else
    {
        write_instruction(result + " = icmp " + comparison + " " +
                          llvm_type(type, item.position) + " " + left.text +
                          ", " + right.text);
    }
    return {item.type, result};
}

} // namespace tx
