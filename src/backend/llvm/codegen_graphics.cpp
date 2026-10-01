#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

void llvm_code_generator::write_graphics_declarations()
{
    write_gui_declarations();
    module_ << "declare ptr @txrt_graphics_resource_view(ptr)\n"
        << "declare i32 @txrt_graphics_open_app(ptr, ptr)\n"
        << "declare i32 @txrt_graphics_close_app(ptr)\n"
        << "declare i32 @txrt_graphics_close_window(ptr)\n"
        << "declare i32 @txrt_graphics_create_window(ptr, ptr, double, double, i1, ptr, ptr)\n"
        << "declare i32 @txrt_graphics_is_open(ptr, ptr)\n"
        << "declare i32 @txrt_graphics_show(ptr, ptr, ptr)\n"
        << "declare i32 @txrt_graphics_set_title(ptr, ptr, ptr, ptr)\n"
        << "declare i32 @txrt_graphics_window_id(ptr, ptr)\n"
        << "declare i32 @txrt_graphics_invalidate(ptr)\n"
        << "declare i32 @txrt_graphics_backend(ptr, ptr)\n"
        << "declare i32 @txrt_graphics_last_native_error(ptr)\n"
        << "declare i32 @txrt_graphics_next_event(ptr, i64, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr)\n"
        << "declare i32 @txrt_graphics_begin_frame(ptr, ptr, ptr)\n"
        << "declare i32 @txrt_graphics_end_frame(ptr, ptr, ptr)\n"
        << "declare i32 @txrt_graphics_cancel_frame(ptr)\n"
        << "declare i32 @txrt_graphics_clear(ptr, double, double, double, double)\n";
    for (const auto* name : {"draw_line", "draw_rect", "fill_rect", "draw_ellipse",
        "fill_ellipse", "draw_rounded_rect", "fill_rounded_rect"})
    {
        const std::string_view operation(name);
        const auto count = 8 + (operation.starts_with("draw_") ? 1 : 0) +
            (operation.ends_with("rounded_rect") ? 2 : 0);
        module_ << "declare i32 @txrt_graphics_" << name << "(ptr";
        for (int index = 0; index < count; ++index)
        {
            module_ << ", double";
        }
        module_ << ")\n";
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
    if (item.type.is_result())
    {
        append("ptr " + global_bytes(item.type.name));
    }
    else if (target.external_name.starts_with("gui.") && structs_.contains(item.type.name))
    {
        append("ptr @tx_record_" + item.type.name);
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
    const bool gui = target.external_name.starts_with("gui.");
    auto symbol = gui ? "txrt_gui_" + target.external_name.substr(4) :
        "txrt_graphics_" + target.external_name.substr(9);
    if (gui)
    {
        // 重载由 sema 解析，资源控制块的公共实现无需再判断控件类型。
        const auto operation = target.external_name.substr(4);
        for (const auto* name : {"id", "is_open", "close", "set_enabled", "set_visible", "set_reserved_space", "set_width", "set_height", "set_constraints", "set_margin", "set_alignment", "set_cell", "set_text", "text", "focus"})
        {
            if (operation == name)
            {
                symbol += "_" + target.parameters.front().type.name.substr(4);
                break;
            }
        }
    }
    if (target.external_name == "graphics.close")
    {
        symbol += target.parameters.front().type.name == "graphics_app" ? "_app" : "_window";
    }
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
