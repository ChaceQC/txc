#include "backend/llvm/codegen.hpp"
#include <algorithm>

namespace tx
{

std::string llvm_code_generator::allocate_record_data(const value_type& type)
{
    const auto result = "%slot" + std::to_string(next_slot_++);
    allocations_ << "  " << result << " = alloca [" << structs_.at(type.name)->fields.size()
                 << " x i64], align 8\n";
    return result;
}

void llvm_code_generator::copy_record_data(const value_type& type,
    const std::string& source, const std::string& target)
{
    for (std::size_t index = 0; index < structs_.at(type.name)->fields.size(); ++index)
    {
        const auto from = temporary();
        const auto to = temporary();
        const auto value = temporary();
        write_instruction(from + " = getelementptr i64, ptr " + source + ", i64 " + std::to_string(index));
        write_instruction(to + " = getelementptr i64, ptr " + target + ", i64 " + std::to_string(index));
        write_instruction(value + " = load i64, ptr " + from);
        write_instruction("store i64 " + value + ", ptr " + to);
    }
}

std::string llvm_code_generator::native_record_data(const expression& item, std::vector<ir_value>& owned)
{
    if (const auto* name = std::get_if<name_reference>(&item.data))
    {
        const auto slot = find_variable(name->name, item.position);
        if (!slot.stack_record.empty())
        {
            return slot.stack_record;
        }
    }
    if (native_record_expression(item))
    {
        if (const auto* binary = std::get_if<binary_operation>(&item.data))
        {
            return native_record_call(*native_record_target(*binary->binding), binary->left.get(),
                                      {binary->right.get()}, item.position).text;
        }
        const auto& call = std::get<call_expression>(item.data);
        if (call.is_constructor)
        {
            const auto result = allocate_record_data(item.type);
            for (std::size_t index = 0; index < call.arguments.size(); ++index)
            {
                const auto& argument = call.arguments[index];
                const auto field = argument.kind == argument_kind::keyword
                    ? field_index(item.type, argument.name, argument.position) : index;
                const auto value = expression_value(*argument.value);
                const auto address = temporary();
                write_instruction(address + " = getelementptr i64, ptr " + result +
                                  ", i64 " + std::to_string(field));
                // bool 的高位也初始化，整体拷贝槽位时不读取未初始化的字节。
                write_instruction("store i64 0, ptr " + address);
                write_instruction("store " + llvm_type(value.type, item.position) + " " + value.text +
                                  ", ptr " + address);
            }
            return result;
        }
        std::vector<const expression*> arguments;
        for (const auto& argument : call.arguments)
        {
            arguments.push_back(argument.value.get());
        }
        return native_record_call(*native_record_target(call), call.receiver.get(), arguments, item.position).text;
    }
    bool borrowed = false;
    const auto value = expression_value_or_borrow(item, borrowed);
    if (!borrowed)
    {
        owned.push_back(value);
    }
    const auto view = record_view_value(value);
    const auto data = temporary();
    write_instruction(data + " = load ptr, ptr " + view);
    return data;
}

llvm_code_generator::ir_value llvm_code_generator::native_record_call(const function_decl& function,
    const expression* receiver, const std::vector<const expression*>& arguments, source_pos position)
{
    const bool record_result = scalar_record_type(function.return_type);
    const auto output = record_result ? allocate_record_data(function.return_type) : temporary();
    std::string parameters = "ptr %tx_context";
    if (record_result)
    {
        parameters += ", ptr " + output;
    }
    std::vector<ir_value> owned;
    if (receiver)
    {
        parameters += ", ptr " + native_record_data(*receiver, owned);
    }
    for (const auto* argument : arguments)
    {
        parameters += ", " + llvm_type(argument->type, position) + " ";
        if (scalar_record_type(argument->type))
        {
            parameters += native_record_data(*argument, owned);
        }
        else
        {
            parameters += expression_value(*argument).text;
        }
    }
    const auto symbol = function.owner_class.empty() ? function.name :
        class_method_symbol(function.owner_class, function.name);
    const auto& overloads = functions_.at(symbol);
    const auto overload = static_cast<std::size_t>(std::find(overloads.begin(), overloads.end(), &function) -
                                                   overloads.begin());
    emit_stack_location();
    write_instruction((record_result ? "call void " : output + " = call " +
        llvm_type(function.return_type, position) + " ") + function_name(symbol, overload) +
        "_native(" + parameters + ")");
    for (const auto& value : owned)
    {
        release(value);
    }
    return {function.return_type, output, record_result};
}

bool llvm_code_generator::emit_native_record_assignment(const variable_assignment& assignment)
{
    const auto* name = std::get_if<name_reference>(&assignment.target->data);
    if (!name)
    {
        return false;
    }
    const auto slot = find_variable(name->name, assignment.target->position);
    if (slot.stack_record.empty())
    {
        return false;
    }
    std::vector<ir_value> owned;
    const auto source = assignment.binding
        ? native_record_call(*native_record_target(*assignment.binding), assignment.target.get(),
                             {assignment.value.get()}, assignment.target->position).text
        : native_record_data(*assignment.value, owned);
    copy_record_data(slot.type, source, slot.stack_record);
    for (const auto& value : owned)
    {
        release(value);
    }
    return true;
}

} // namespace tx
