#include "backend/cpp/native_gui_abi.hpp"
#include "backend/cpp/native_gui_result.hpp"

using namespace tx_generated;
using graphics::resource;

namespace
{
int open_application(const std::string& font, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::open_app(font));
    });
}

dynamic_struct event_value(const native_gui::event& event, const char* type)
{
    struct_fields fields(7);
    fields[0] = {"kind", event.kind};
    fields[1] = {"window_id", event.window_id};
    fields[2] = {"source_id", event.source_id};
    fields[3] = {"text", event.text};
    fields[4] = {"number", event.number};
    fields[5] = {"state", event.state};
    fields[6] = {"revision", event.revision};
    return dynamic_struct(dynamic_struct_data{type, "event", std::move(fields)});
}
}

extern "C" int txrt_native_gui_open_app(const char* type, void** result) noexcept
{
    return open_application({}, type, result);
}

extern "C" int txrt_native_gui_open_app_with_font(const void* font, const char* type, void** result) noexcept
{
    return open_application(detail::text_value(font), type, result);
}

extern "C" int txrt_native_gui_create_window(resource* app, const void* title,
    double width, double height, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::create_window(native_gui::require_app(app),
            detail::text_value(title), width, height));
    });
}

extern "C" int txrt_native_gui_show(resource* window) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& value = native_gui::require_window(window);
        value.host->show();
        value.visible = true;
        value.repaint = true;
    });
}

extern "C" int txrt_native_gui_set_title(resource* window, const void* title) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::require_window(window).host->set_title(detail::text_value(title));
    });
}

extern "C" int txrt_native_gui_is_open(resource* window, bool* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        const auto& state = native_gui::require_window(window, true);
        *result = !state.closed && state.host->is_open();
    });
}

extern "C" int txrt_native_gui_close_window(resource* window) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::close_window(native_gui::require_window(window, true));
    });
}

extern "C" int txrt_native_gui_close_app(resource* app) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::close_app(native_gui::require_app(app, true));
    });
}

extern "C" int txrt_native_gui_next_event(resource* app, std::int64_t timeout,
    const char* type, const char* event_type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        const auto pending = native_gui::next_event(native_gui::require_app(app), timeout);
        return graphics::option_value(std::string("option<") + event_type + ">",
            pending ? std::any(event_value(*pending, event_type)) : std::any{});
    });
}
