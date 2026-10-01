#include "backend/cpp/graphics_abi.hpp"
#include "backend/cpp/graphics_result.hpp"
#include "stdlib/graphics/windows/state.hpp"

using namespace tx_generated;
using namespace tx_generated::graphics;

namespace
{

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
    fields[8] = {"control", option_value(control_type)};
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
