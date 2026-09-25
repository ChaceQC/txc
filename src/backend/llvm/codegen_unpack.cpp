#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::emit_unpack(
    const statement& item, const unpack_assignment& assignment)
{
    if (const auto* name = std::get_if<name_reference>(&assignment.value->data))
    {
        const auto array = find_variable(name->name, item.position);
        if (array.local_array_length)
        {
            emit_local_array_unpack(item, assignment, array);
            return;
        }
    }
    bool borrowed_source = false;
    const auto source = expression_value_or_borrow(*assignment.value,
                                                   borrowed_source);
    std::string array_reference;
    if (borrowed_source && source.type == value_type::array_type)
    {
        if (const auto* name =
                std::get_if<name_reference>(&assignment.value->data))
        {
            const auto variable = find_variable(name->name, item.position);
            if (!variable.array_reference.empty())
            {
                array_reference = load_array_reference(variable);
            }
        }
    }
    const auto length_status = temporary();
    write_instruction(length_status +
        " = call i32 @txrt_array_require_length(ptr " + source.text +
        ", i64 " + std::to_string(assignment.names.size()) + ")");
    write_instruction("call void @txrt_require_success(i32 " +
                      length_status + ")");
    std::vector<ir_value> values;
    std::vector<std::optional<variable_slot>> snapshots;
    values.reserve(assignment.names.size());
    snapshots.reserve(assignment.names.size());
    for (std::size_t index = 0; index < assignment.names.size(); ++index)
    {
        const auto type = assignment.declares[index]
            ? value_type::any_type
            : find_variable(assignment.names[index], item.position).type;
        const auto borrowed = temporary();
        write_instruction(borrowed +
            " = call ptr @" + std::string(array_reference.empty()
                ? "txrt_array_element_read_ptr"
                : "txrt_array_ref_element_read_ptr") + "(ptr " +
            (array_reference.empty() ? source.text : array_reference) +
            ", i64 " + std::to_string(index) + ")");
        if (type != value_type::any_type)
        {
            const auto checked = temporary();
            write_instruction(checked + " = call i32 @txrt_value_require_type(ptr " +
                              borrowed + ", ptr " + global_bytes(type.name) + ")");
            write_instruction("call void @txrt_require_success(i32 " +
                              checked + ")");
        }
        if (assignment.declares[index] &&
            !rebinds_name(*current_function_body_, assignment.names[index]))
        {
            snapshots.push_back(make_scalar_snapshot(borrowed, item.position));
            values.push_back({value_type::void_type, {}});
        }
        else
        {
            snapshots.push_back(std::nullopt);
            values.push_back(from_any({value_type::any_type, borrowed},
                                      type, item.position));
        }
    }
    for (std::size_t index = 0; index < assignment.names.size(); ++index)
    {
        if (assignment.declares[index])
        {
            if (snapshots[index])
            {
                scopes_.back().emplace(assignment.names[index],
                                       *snapshots[index]);
                continue;
            }
            const auto address = allocate(value_type::any_type, item.position);
            write_instruction("store ptr " + values[index].text + ", ptr " + address);
            scopes_.back().emplace(assignment.names[index],
                variable_slot{value_type::any_type, address});
        }
        else
        {
            const auto slot = find_variable(assignment.names[index], item.position);
            store_variable(slot, values[index], item.position);
        }
    }
    if (!borrowed_source)
    {
        release(source);
    }
}

} // namespace tx
