#include "backend/llvm/codegen.hpp"

namespace tx
{

std::string llvm_code_generator::container_symbol(const value_type& type,
                                                   std::string_view operation)
{
    std::string symbol = "txrt_" + type.container_name() + "_" + std::string(operation);
    for (const auto& parameter : type.parameters)
    {
        symbol += "_" + vector_suffix(value_type::vector_of(parameter));
    }
    return symbol;
}

llvm_code_generator::ir_value llvm_code_generator::container_operation(
    const value_type& type, std::string_view operation,
    const std::vector<ir_value>& arguments, const value_type& result_type, source_pos position)
{
    std::string parameters;
    for (const auto& argument : arguments)
    {
        parameters += (parameters.empty() ? "" : ", ") +
                      llvm_type(argument.type, position) + " " + argument.text;
    }
    const bool returns_value = result_type != value_type::void_type;
    const auto output = returns_value ? allocate(result_type, position) : "";
    if (returns_value)
    {
        parameters += (parameters.empty() ? "" : ", ") + std::string("ptr ") + output;
    }
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + container_symbol(type, operation) +
                      "(" + parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    if (!returns_value)
    {
        return {value_type::void_type, {}};
    }
    const auto result = temporary();
    write_instruction(result + " = load " + llvm_type(result_type, position) + ", ptr " + output);
    return {result_type, result};
}

llvm_code_generator::ir_value llvm_code_generator::emit_container_call(
    const expression& item, const call_expression& call)
{
    const auto& type = call.container_type ? *call.container_type : call.receiver->type;
    std::vector<ir_value> arguments;
    if (call.receiver)
    {
        // 接收者必须在后续实参之前持有，避免字段重新绑定使目标容器提前释放。
        arguments.push_back(expression_value(*call.receiver));
    }
    for (const auto& argument : call.arguments)
    {
        arguments.push_back(expression_value(*argument.value));
    }
    if (call.container_type && type.container_name() == "heap" && arguments.empty())
    {
        arguments.push_back({value_type::bool_type, "false"});
    }
    const auto result = container_operation(type, call.container_type ? "new" : call.name,
                                            arguments, item.type, item.position);
    for (const auto& argument : arguments)
    {
        release(argument);
    }
    return result;
}

void llvm_code_generator::initialize_container_fields(
    const class_decl& definition, const std::string& object, source_pos position)
{
    std::vector<const class_decl*> nodes;
    collect_class_nodes(definition, nodes);
    for (const auto* node : nodes)
    {
        for (const auto& field : node->fields)
        {
            if (!field.type.is_typed_container())
            {
                continue;
            }
            std::vector<ir_value> arguments;
            if (field.type.container_name() == "heap")
            {
                arguments.push_back({value_type::bool_type, "false"});
            }
            // 字段默认值同样在编译期选定构造入口，不按类型名称在运行时分派。
            const auto value = container_operation(field.type, "new", arguments, field.type, position);
            const auto address = allocate(value_type::any_type, position);
            const auto status = temporary();
            write_instruction(status + " = call i32 @txrt_class_field_address_index(ptr " +
                object + ", i64 " + std::to_string(field.slot) + ", i1 true, ptr " + address + ")");
            write_instruction("call void @txrt_require_success(i32 " + status + ")");
            const auto slot = temporary();
            write_instruction(slot + " = load ptr, ptr " + address);
            assign_any(slot, value, position);
            release(value);
        }
    }
}

} // namespace tx
