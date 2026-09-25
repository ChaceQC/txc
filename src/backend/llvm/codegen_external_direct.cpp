#include "backend/llvm/codegen.hpp"

#include <algorithm>
#include <string_view>

namespace tx
{
namespace
{

const char* numeric_symbol(std::string_view name, bool integer)
{
    if (name == "math.abs")
    {
        return integer ? "txrt_math_abs_i64" : "txrt_math_abs_f64";
    }
    if (name == "math.min")
    {
        return integer ? "txrt_math_min_i64" : "txrt_math_min_f64";
    }
    if (name == "math.max")
    {
        return integer ? "txrt_math_max_i64" : "txrt_math_max_f64";
    }
    if (name == "math.clamp")
    {
        return integer ? "txrt_math_clamp_i64" : "txrt_math_clamp_f64";
    }
    if (name == "math.mod")
    {
        return "txrt_math_mod_i64";
    }
    if (name == "math.sqrt")
    {
        return "txrt_math_sqrt_f64";
    }
    if (name == "math.pow")
    {
        return "txrt_math_pow_f64";
    }
    if (name == "math.floor")
    {
        return "txrt_math_floor_f64";
    }
    if (name == "math.ceil")
    {
        return "txrt_math_ceil_f64";
    }
    if (name == "random.seed")
    {
        return "txrt_random_seed_context";
    }
    if (name == "random.random_int")
    {
        return "txrt_random_int_i64";
    }
    if (name == "random.random_float")
    {
        return "txrt_random_float_f64";
    }
    return nullptr;
}

} // namespace

std::string llvm_code_generator::random_context()
{
    if (random_context_slot_.empty())
    {
        random_context_slot_ = "%slot" + std::to_string(next_slot_++);
        allocations_ << "  " << random_context_slot_ << " = alloca ptr\n"
                     << "  store ptr null, ptr " << random_context_slot_ << '\n';
    }
    const auto cached = temporary();
    write_instruction(cached + " = load ptr, ptr " + random_context_slot_);
    const auto missing = temporary();
    write_instruction(missing + " = icmp eq ptr " + cached + ", null");
    const auto acquire_label = label();
    const auto ready_label = label();
    write_instruction("br i1 " + missing + ", label %" + acquire_label +
                      ", label %" + ready_label);
    start_block(acquire_label);
    const auto acquired = temporary();
    write_instruction(acquired + " = call ptr @txrt_random_context()");
    write_instruction("store ptr " + acquired + ", ptr " + random_context_slot_);
    write_instruction("br label %" + ready_label);
    start_block(ready_label);
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + random_context_slot_);
    return result;
}

llvm_code_generator::ir_value llvm_code_generator::emit_direct_external_call(
    const expression& item, const function_decl& target,
    const std::vector<ir_value>& arguments)
{
    const bool integer = !arguments.empty() &&
        arguments.front().type == value_type::int_type;
    std::string symbol;
    if (const auto* numeric = numeric_symbol(target.external_name, integer))
    {
        symbol = numeric;
    }
    else
    {
        symbol = "txrt_" + target.external_name;
        std::replace(symbol.begin(), symbol.end(), '.', '_');
    }
    const bool dictionary_key_call = target.external_name == "dictionary.get" ||
        target.external_name == "dictionary.contains" ||
        target.external_name == "dictionary.remove";
    ir_value boxed_key{value_type::void_type, {}};
    if (dictionary_key_call)
    {
        boxed_key = box_any(arguments[1], item.position);
    }
    std::string parameters;
    for (std::size_t index = 0; index < arguments.size(); ++index)
    {
        if (!parameters.empty())
        {
            parameters += ", ";
        }
        const auto& argument = dictionary_key_call && index == 1
            ? boxed_key : arguments[index];
        parameters += llvm_type(argument.type, item.position) + " " + argument.text;
    }
    if (target.external_name.starts_with("random."))
    {
        const auto context = random_context();
        parameters = "ptr " + context + (parameters.empty() ? "" : ", " + parameters);
    }
    if (target.external_name == "random.random_int" ||
        target.external_name == "random.random_float")
    {
        const auto result = temporary();
        const auto* direct = target.external_name == "random.random_int"
            ? "txrt_random_int_context" : "txrt_random_float_context";
        write_instruction(result + " = call " +
                          llvm_type(item.type, item.position) + " @" + direct +
                          "(" + parameters + ")");
        for (const auto& argument : arguments)
        {
            release(argument);
        }
        return {item.type, result};
    }
    const bool returns_value = item.type != value_type::void_type;
    std::string address;
    if (returns_value)
    {
        address = allocate(item.type, item.position);
        if (!parameters.empty())
        {
            parameters += ", ";
        }
        parameters += "ptr " + address;
    }
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + symbol + "(" + parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    release(boxed_key);
    for (const auto& argument : arguments)
    {
        release(argument);
    }
    if (!returns_value)
    {
        return {value_type::void_type, {}};
    }
    if (item.type == value_type::str_type || is_value_handle(item.type))
    {
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + address);
        return {item.type, result};
    }
    return load({item.type, address});
}

} // namespace tx
