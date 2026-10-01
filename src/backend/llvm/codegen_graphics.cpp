#include "backend/llvm/codegen.hpp"

#include <algorithm>
#include <unordered_set>

namespace tx
{
namespace
{

std::string graphics_symbol(const function_decl& target)
{
    auto symbol = "txrt_" + target.external_name;
    std::replace(symbol.begin(), symbol.end(), '.', '_');
    const auto dot = target.external_name.find('.');
    const auto module = target.external_name.substr(0, dot);
    const auto operation = target.external_name.substr(dot + 1);
    if (module == "graphics")
    {
        if (operation == "close" || operation == "save_png")
        {
            symbol += "_" + target.parameters.front().type.name.substr(9);
            if (operation == "close" && target.parameters.front().type.name == "graphics_path")
            {
                symbol += "_resource";
            }
            if (operation == "save_png" && target.parameters.size() == 2)
            {
                symbol += "_safe";
            }
        }
        else if (operation.starts_with("draw_") || operation.starts_with("fill_"))
        {
            if (operation != "draw_path" && operation != "fill_path" &&
                std::any_of(target.parameters.begin(), target.parameters.end(), [](const auto& parameter)
                {
                    return parameter.type.name == "graphics_brush";
                }))
            {
                symbol += "_brush";
            }
        }
    }
    else if (module == "gui" || module == "gui_data")
    {
        for (const auto* name : {"id", "is_open", "close", "set_enabled", "enabled",
            "set_visible", "set_reserved_space", "set_width", "set_height", "set_constraints",
            "set_margin", "set_alignment", "set_cell", "set_text", "text", "focus",
            "set_checked", "checked", "revision", "count", "replace_items", "append_items",
            "update_items", "remove_items", "set_order", "begin_page", "apply_page",
            "selected_ids", "set_selected_ids", "set_accessibility", "set_label"})
        {
            if (operation == name)
            {
                // 保持 U1 已发行复选框 ABI 的固定名称。
                if ((operation == "checked" || operation == "set_checked") &&
                    target.parameters.front().type.name == "gui_check_box")
                {
                    break;
                }
                symbol += "_" + target.parameters.front().type.name.substr(4);
                break;
            }
        }
    }
    return symbol;
}

} // namespace

void llvm_code_generator::write_graphics_declarations()
{
    module_ << "declare ptr @txrt_graphics_resource_view(ptr)\n";
    std::unordered_set<std::string> declared;
    for (const auto& [name, overloads] : functions_)
    {
        for (const auto* function : overloads)
        {
            const auto& external = function->external_name;
            if (!external.starts_with("graphics.") && !external.starts_with("graphics_text.") &&
                !external.starts_with("gui.") && !external.starts_with("gui_data."))
            {
                continue;
            }
            const auto symbol = graphics_symbol(*function);
            if (!declared.insert(symbol).second)
            {
                continue;
            }
            std::vector<std::string> parameters;
            for (const auto& parameter : function->parameters)
            {
                if (const auto record = structs_.find(parameter.type.name); record != structs_.end())
                {
                    for (const auto& field : record->second->fields)
                    {
                        parameters.push_back(llvm_type(field.type, function->position));
                    }
                }
                else
                {
                    parameters.push_back(llvm_type(parameter.type, function->position));
                }
            }
            const auto& result = function->return_type;
            if (result.is_result() || result.is_option() || structs_.contains(result.name) ||
                (result.is_vector() && structs_.contains(result.parameters.front().name)))
            {
                parameters.push_back("ptr");
            }
            if (external == "graphics.next_event")
            {
                parameters.insert(parameters.end(), 6, "ptr");
            }
            if (result != value_type::void_type)
            {
                parameters.push_back("ptr");
            }
            module_ << "declare i32 @" << symbol << "(";
            for (std::size_t index = 0; index < parameters.size(); ++index)
            {
                module_ << (index ? ", " : "") << parameters[index];
            }
            module_ << ")\n";
        }
    }
}

llvm_code_generator::ir_value llvm_code_generator::emit_graphics_call(
    const expression& item, const function_decl& target, const std::vector<ir_value>& arguments)
{
    std::string parameters;
    const auto append = [&](const std::string& value)
    {
        parameters += (parameters.empty() ? "" : ", ") + value;
    };
    std::vector<ir_value> fields;
    for (const auto& argument : arguments)
    {
        if (argument.type.is_graphics_resource())
        {
            append("ptr " + record_view_value(argument));
        }
        else if (structs_.contains(argument.type.name))
        {
            auto record = argument;
            record.record_view = record_view_value(record);
            const auto& definitions = structs_.at(record.type.name)->fields;
            for (std::size_t index = 0; index < definitions.size(); ++index)
            {
                const auto address = record_field_slot(record, index);
                const auto& type = definitions[index].type;
                ir_value field;
                if (type == value_type::str_type)
                {
                    const auto reference = temporary();
                    write_instruction(reference + " = load ptr, ptr " + address);
                    field = from_any({value_type::any_type, reference}, type, item.position);
                    fields.push_back(field);
                }
                else
                {
                    field = load({type, address});
                }
                append(llvm_type(type, item.position) + " " + field.text);
            }
        }
        else
        {
            append(llvm_type(argument.type, item.position) + " " + argument.text);
        }
    }
    if (item.type.is_result() || item.type.is_option())
    {
        append("ptr " + global_bytes(item.type.name));
    }
    else if (structs_.contains(item.type.name))
    {
        append("ptr @tx_record_" + item.type.name);
    }
    else if (item.type.is_vector() && structs_.contains(item.type.parameters.front().name))
    {
        append("ptr @tx_record_" + item.type.parameters.front().name);
    }
    if (target.external_name == "graphics.next_event")
    {
        const auto& event_type = item.type.parameters.front().parameters.front();
        const auto& definitions = structs_.at(event_type.name)->fields;
        append("ptr " + global_bytes(event_type.name));
        append("ptr " + global_bytes(definitions[6].type.parameters.front().name));
        for (const auto index : {3, 4, 5, 8})
        {
            append("ptr " + global_bytes(definitions[index].type.name));
        }
    }
    const bool returns_value = item.type != value_type::void_type;
    const auto output = returns_value ? allocate(item.type, item.position) : std::string{};
    if (returns_value)
    {
        append("ptr " + output);
    }
    const auto symbol = graphics_symbol(target);
    const auto status = temporary();
    write_instruction(status + " = call i32 @" + symbol + "(" + parameters + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    for (const auto& field : fields)
    {
        release(field);
    }
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
        write_instruction(result + " = load ptr, ptr " + output);
        return {item.type, result};
    }
    return load({item.type, output});
}

} // namespace tx
