#include "backend/llvm/codegen.hpp"

namespace tx
{

bool llvm_code_generator::emit_regex_projection(const statement& item,
    const variable_declaration& declaration)
{
    const auto* call = declaration.initializer
        ? std::get_if<call_expression>(&declaration.initializer->data) : nullptr;
    if (!call || call->indirect || call->receiver || !call->overload_index)
    {
        return false;
    }
    const auto found = functions_.find(call->name);
    if (found == functions_.end() || *call->overload_index >= found->second.size())
    {
        return false;
    }
    const auto& target = *found->second[*call->overload_index];
    const auto mode = target.external_name == "regex.search" ? 0 :
        target.external_name == "regex.match" ? 1 : target.external_name == "regex.full_match" ? 2 : -1;
    if (mode < 0 || !readonly_local_fields(declaration) || call->arguments.size() < 2)
    {
        return false;
    }
    std::vector<ir_value> values;
    for (std::size_t index = 0; index < call->arguments.size(); ++index)
    {
        if (call->arguments[index].kind != argument_kind::positional)
        {
            return false;
        }
    }
    for (std::size_t index = 0; index < call->arguments.size(); ++index)
    {
        values.push_back(call_argument_value(*call, index));
    }
    const auto text = allocate(value_type::str_type, item.position);
    const auto present = allocate(value_type::bool_type, item.position);
    const auto offsets = "%slot" + std::to_string(next_slot_++);
    allocations_ << "  " << offsets << " = alloca [4 x i64], align 8\n";
    const auto status = temporary();
    const auto start = mode == 0 && values.size() > 2 ? values[2].text : "0";
    write_instruction(status + " = call i32 @txrt_regex_projected(ptr " + values[0].text +
        ", ptr " + values[1].text + ", i64 " + start + ", i64 " + std::to_string(mode) +
        ", ptr " + present + ", ptr " + text + ", ptr " + offsets + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    for (const auto& value : values)
    {
        release(value);
    }
    variable_slot slot{declaration.initializer->type, text};
    slot.projected_fields.emplace("text", text);
    slot.projected_fields.emplace("found", present);
    std::size_t index = 0;
    for (const auto* field : {"start_byte", "end_byte", "start_scalar", "end_scalar"})
    {
        const auto address = temporary();
        write_instruction(address + " = getelementptr i64, ptr " + offsets + ", i64 " + std::to_string(index++));
        slot.projected_fields.emplace(field, address);
    }
    scopes_.back().emplace(declaration.name, std::move(slot));
    return true;
}

std::optional<llvm_code_generator::ir_value> llvm_code_generator::emit_projected_field(const expression& item)
{
    const auto* member = std::get_if<member_expression>(&item.data);
    const auto* name = member ? std::get_if<name_reference>(&member->object->data) : nullptr;
    if (!name)
    {
        return std::nullopt;
    }
    const auto slot = find_variable(name->name, member->object->position);
    const auto found = slot.projected_fields.find(member->field);
    return found == slot.projected_fields.end() ? std::nullopt :
        std::optional<ir_value>(load({item.type, found->second}));
}

} // namespace tx
