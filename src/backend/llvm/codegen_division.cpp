#include "backend/llvm/codegen.hpp"

#include <limits>
#include <algorithm>

namespace tx
{

llvm_code_generator::ir_value llvm_code_generator::emit_division(
    const std::string& name, const ir_value& left, const ir_value& right,
    source_pos position)
{
    const bool integer = left.type == value_type::int_type;
    const auto type = integer ? "i64" : "double";
    const auto divisor = integer_range_of(right);
    const auto dividend = integer_range_of(left);
    const bool range_safe = integer && (divisor.second < 0 || divisor.first > 0) &&
        (divisor.first > -1 || divisor.second < -1 ||
         dividend.first > std::numeric_limits<std::int64_t>::min());
    if (!range_safe && !proven_integer_function_)
    {
        const auto zero = temporary();
        write_instruction(zero + (integer ? " = icmp eq i64 " : " = fcmp oeq double ") +
            right.text + (integer ? ", 0" : ", 0.0"));
        auto invalid = zero;
        if (integer)
        {
            const auto minimum = temporary();
            const auto negative_one = temporary();
            const auto overflow = temporary();
            invalid = temporary();
            write_instruction(minimum + " = icmp eq i64 " + left.text + ", -9223372036854775808");
            write_instruction(negative_one + " = icmp eq i64 " + right.text + ", -1");
            write_instruction(overflow + " = and i1 " + minimum + ", " + negative_one);
            write_instruction(invalid + " = or i1 " + zero + ", " + overflow);
        }
        const auto error = label();
        const auto ready = label();
        write_instruction("br i1 " + invalid + ", label %" + error + ", label %" + ready);
        start_block(error);
        const auto output = allocate(left.type, position);
        const auto status = temporary();
        write_instruction(status + " = call i32 @" + name + "(" + type + " " +
            left.text + ", " + type + " " + right.text + ", ptr " + output + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        write_instruction("unreachable");
        start_block(ready);
    }
    const auto result = temporary();
    write_instruction(result + (integer ? " = sdiv i64 " : " = fdiv double ") +
        left.text + ", " + right.text);
    ir_value value{left.type, result};
    if (range_safe)
    {
        const auto bounds = std::minmax({dividend.first / divisor.first, dividend.first / divisor.second,
                                        dividend.second / divisor.first, dividend.second / divisor.second});
        value.integer_range = integer_interval{bounds.first, bounds.second};
    }
    return value;
}

} // namespace tx
