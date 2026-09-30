#include "backend/llvm/codegen.hpp"

namespace tx
{

bool llvm_code_generator::direct_scalar_field(const expression& item) const
{
    const auto* member = std::get_if<member_expression>(&item.data);
    if (!member ||
        !std::holds_alternative<name_reference>(member->object->data) ||
        (item.type != value_type::int_type &&
         item.type != value_type::float_type &&
         item.type != value_type::bool_type))
    {
        return false;
    }
    const auto& owner = member->object->type.name;
    return classes_.contains(owner) ? member->field_slot.has_value()
                                    : structs_.contains(owner);
}

std::string llvm_code_generator::scalar_field_address(
    const expression& item, bool read_only)
{
    const auto& member = std::get<member_expression>(item.data);
    const auto& name = std::get<name_reference>(member.object->data);
    const auto variable = find_variable(name.name, item.position);
    if (const auto found = variable.projected_fields.find(member.field);
        found != variable.projected_fields.end())
    {
        return found->second;
    }
    if (static_record_type(member.object->type))
    {
        const auto index = member.field_slot.value_or(classes_.contains(member.object->type.name)
            ? 0 : field_index(member.object->type, member.field, item.position));
        auto data = variable.stack_record;
        if (data.empty())
        {
            auto view = temporary();
            if (variable.record_view.empty())
            {
                const auto handle = temporary();
                write_instruction(handle + " = load ptr, ptr " + variable.address);
                view = record_view_value({variable.type, handle});
            }
            else
            {
                write_instruction(view + " = load ptr, ptr " + variable.record_view);
            }
            data = temporary();
            write_instruction(data + " = load ptr, ptr " + view);
        }
        const auto address = temporary();
        write_instruction(address + " = getelementptr inbounds i64, ptr " + data +
                          ", i64 " + std::to_string(index));
        return address;
    }
    if (!variable.native_parse_ok.empty())
    {
        if (read_only)
        {
            return native_parse_field_address(variable, item);
        }
        materialize_native_parse(variable);
    }
    const bool class_field = classes_.contains(member.object->type.name);
    const auto slot = class_field ? *member.field_slot :
        field_index(member.object->type, member.field, item.position);
    if (name.name == "self")
    {
        // self 不可重新绑定，且字段槽位固定；入口块取得的地址支配后续代码块。
        if (const auto found = entry_scalar_field_cache_.find(slot);
            found != entry_scalar_field_cache_.end())
        {
            return found->second;
        }
    }
    const auto object = temporary();
    write_instruction(object + " = load ptr, ptr " + variable.address);
    const auto* function = class_field
        ? (item.type == value_type::int_type ? "txrt_class_field_i64_ptr"
           : item.type == value_type::float_type ? "txrt_class_field_f64_ptr"
           : "txrt_class_field_bool_ptr")
        : (item.type == value_type::int_type ? "txrt_struct_field_i64_ptr"
           : item.type == value_type::float_type ? "txrt_struct_field_f64_ptr"
           : "txrt_struct_field_bool_ptr");
    const auto address = temporary();
    write_instruction(address + " = call ptr @" + function + "(ptr " +
                      object + ", i64 " + std::to_string(slot) + ")");
    if (name.name == "self" && in_entry_block_)
    {
        entry_scalar_field_cache_.emplace(slot, address);
    }
    return address;
}

} // namespace tx
