#include "backend/cpp/graphics_abi.hpp"
#include "backend/cpp/graphics_result.hpp"
#include "stdlib/graphics/windows/state.hpp"

using namespace tx_generated;
using namespace tx_generated::graphics;

namespace
{

dynamic_struct control_value(const char* option_type, const event::control_data& item)
{
    const std::string option(option_type);
    const auto type = option.substr(7, option.size() - 8);
    struct_fields fields(8);
    fields[0] = {"source_id", item.source_id};
    fields[1] = {"action", item.action};
    fields[2] = {"text", option_value("option<str>",
        item.text ? std::any(*item.text) : std::any{})};
    fields[3] = {"number", option_value("option<float>",
        item.number ? std::any(*item.number) : std::any{})};
    fields[4] = {"state", option_value("option<bool>",
        item.state ? std::any(*item.state) : std::any{})};
    fields[5] = {"item_id", option_value("option<int>",
        item.item_id ? std::any(*item.item_id) : std::any{})};
    fields[6] = {"command_id", option_value("option<int>",
        item.command_id ? std::any(*item.command_id) : std::any{})};
    fields[7] = {"revision", item.revision};
    return dynamic_struct(dynamic_struct_data{type, "control_event", std::move(fields)});
}

dynamic_struct resize_value(const char* type, const size_event& item)
{
    struct_fields fields(5);
    fields[0] = {"width", item.width};
    fields[1] = {"height", item.height};
    fields[2] = {"pixel_width", item.pixel_width};
    fields[3] = {"pixel_height", item.pixel_height};
    fields[4] = {"dpi", item.dpi};
    return dynamic_struct(dynamic_struct_data{type, "resize_event", std::move(fields)});
}

dynamic_struct event_value(const event& item, const char* type, const char* resize_type,
    const char* pointer_type, const char* key_type, const char* text_type, const char* control_type)
{
    struct_fields fields(9);
    fields[0] = {"kind", item.kind};
    fields[1] = {"window_id", item.window_id};
    fields[2] = {"timestamp_ms", item.timestamp_ms};
    fields[3] = {"pointer", option_value(pointer_type)};
    fields[4] = {"key", option_value(key_type)};
    fields[5] = {"text", option_value(text_type)};
    fields[6] = {"resize", option_value(std::string("option<") + resize_type + ">",
        item.resize ? std::any(resize_value(resize_type, *item.resize)) : std::any{})};
    fields[7] = {"timer_id", option_value("option<int>")};
    fields[8] = {"control", option_value(control_type,
        item.control ? std::any(control_value(control_type, *item.control)) : std::any{})};
    const auto payload = [](const char* option, const char* name, struct_fields values)
    {
        const std::string type(option);
        return option_value(type, dynamic_struct(dynamic_struct_data{
            type.substr(7, type.size() - 8), name, std::move(values)}));
    };
    if (item.pointer)
    {
        const auto& data = *item.pointer;
        struct_fields values(9);
        values[0] = {"x", data.x};
        values[1] = {"y", data.y};
        values[2] = {"wheel_x", data.wheel_x};
        values[3] = {"wheel_y", data.wheel_y};
        values[4] = {"button", data.button};
        values[5] = {"shift", data.shift};
        values[6] = {"ctrl", data.ctrl};
        values[7] = {"alt", data.alt};
        values[8] = {"meta", data.meta};
        fields[3] = {"pointer", payload(pointer_type, "pointer_event", std::move(values))};
    }
    if (item.key)
    {
        const auto& data = *item.key;
        struct_fields values(7);
        values[0] = {"key", data.key};
        values[1] = {"scan_code", data.scan_code};
        values[2] = {"shift", data.shift};
        values[3] = {"ctrl", data.ctrl};
        values[4] = {"alt", data.alt};
        values[5] = {"meta", data.meta};
        values[6] = {"repeat", data.repeat};
        fields[4] = {"key", payload(key_type, "key_event", std::move(values))};
    }
    if (item.text)
    {
        struct_fields values(3);
        values[0] = {"text", item.text->text};
        values[1] = {"selection_start", item.text->selection_start};
        values[2] = {"selection_length", item.text->selection_length};
        fields[5] = {"text", payload(text_type, "text_event", std::move(values))};
    }
    fields[7] = {"timer_id", option_value("option<int>",
        item.timer_id ? std::any(*item.timer_id) : std::any{})};
    return dynamic_struct(dynamic_struct_data{type, "event", std::move(fields)});
}

} // namespace

extern "C" int txrt_graphics_next_event(resource* value, std::int64_t timeout,
    const char* type, const char* event_type, const char* resize_type, const char* pointer_type,
    const char* key_type, const char* text_type, const char* control_type, void** result) noexcept
{
    return result_call(type, result, [&]() -> std::any
    {
        const auto pending = next_event(require_app(value), timeout);
        return option_value(std::string("option<") + event_type + ">", pending ?
            std::any(event_value(*pending, event_type, resize_type, pointer_type, key_type,
                text_type, control_type)) : std::any{});
    });
}
