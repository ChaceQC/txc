#include "backend/llvm/codegen.hpp"
#include "common/local_resource_layout.hpp"

namespace tx
{

bool llvm_code_generator::emit_local_guard(const statement& item,
    const variable_declaration& declaration)
{
    const auto* call = declaration.initializer
        ? std::get_if<call_expression>(&declaration.initializer->data) : nullptr;
    if (!call || call->indirect || call->receiver || !call->overload_index || call->arguments.size() != 1)
    {
        return false;
    }
    const auto found = functions_.find(call->name);
    if (found == functions_.end() || *call->overload_index >= found->second.size() ||
        found->second[*call->overload_index]->external_name != "sync.lock")
    {
        return false;
    }
    const auto& type = declaration.initializer->type;
    if (type.parameters.empty())
    {
        return false;
    }
    const auto& element = type.parameters.front();
    const std::string suffix = element == value_type::int_type ? "i64" :
        element == value_type::float_type ? "f64" : element == value_type::bool_type ? "bool" : "";
    if (suffix.empty() || !confined_guard_local(declaration))
    {
        return false;
    }
    const auto storage = "%slot" + std::to_string(next_slot_++);
    const auto layout = "[" + std::to_string(local_guard_bytes) + " x i8]";
    allocations_ << "  " << storage << " = alloca " << layout << ", align " << local_guard_alignment << '\n'
                 << "  store " << layout << " zeroinitializer, ptr " << storage << '\n';
    if (recoverable_errors_)
    {
        error_roots_.push_back({type, storage, scopes_.size(), "txrt_sync_local_destroy_" + suffix});
    }
    const auto owner = call_argument_value(*call, 0);
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_sync_local_lock_" + suffix +
        "(ptr " + owner.text + ", ptr " + storage + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    release(owner);
    variable_slot slot{type, storage, true};
    slot.local_guard = suffix;
    scopes_.back().emplace(declaration.name, std::move(slot));
    (void)item;
    return true;
}

std::optional<llvm_code_generator::ir_value> llvm_code_generator::emit_local_guard_call(
    const expression& item, const call_expression& call)
{
    if (call.indirect || call.receiver || !call.overload_index || call.arguments.empty())
    {
        return std::nullopt;
    }
    const auto* name = std::get_if<name_reference>(&call.arguments.front().value->data);
    if (!name || !call.arguments.front().value->type.is_sync_value())
    {
        return std::nullopt;
    }
    const auto slot = find_variable(name->name, item.position);
    if (slot.local_guard.empty())
    {
        return std::nullopt;
    }
    const auto& target = *functions_.at(call.name).at(*call.overload_index);
    const auto operation = target.external_name.substr(std::string("sync.guard_").size());
    std::string arguments = "ptr " + slot.address;
    if (operation == "set")
    {
        const auto value = expression_value(*call.arguments.at(1).value);
        arguments += ", " + llvm_type(value.type, item.position) + " " + value.text;
    }
    const auto output = operation == "get" ? allocate(item.type, item.position) : "";
    if (!output.empty())
    {
        arguments += ", ptr " + output;
    }
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_sync_local_" + operation + "_" + slot.local_guard +
        "(" + arguments + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    return output.empty() ? ir_value{value_type::void_type, {}} : load({item.type, output});
}

} // namespace tx
