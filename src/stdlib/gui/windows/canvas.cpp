#include "stdlib/gui/windows/complex.hpp"

#include <algorithm>

namespace tx_generated::gui
{
void canvas_paint(node& state)
{
    const auto app = owner_window(state).owner.lock();
    if (std::any_of(app->events.begin(), app->events.end(), [&](const graphics::event& event)
        {
            return event.control && event.control->source_id == state.id &&
                event.control->action == "canvas_paint";
        }))
    {
        return;
    }
    notify(state, "canvas_paint");
}

void sync_canvas(node& state)
{
    if (state.kind != tx::graphics_kind::gui_canvas || state.closed)
    {
        return;
    }
    auto& owner = owner_window(state);
    if (!state.canvas_window)
    {
        auto window = std::make_shared<graphics::window>();
        window->owner = owner.owner;
        window->id = owner.id;
        window->hwnd = state.hwnd;
        window->gui_canvas_source = state.shared_from_this();
        state.canvas_window = std::move(window);
    }
    auto& window = *state.canvas_window;
    const auto old_width = window.width;
    const auto old_height = window.height;
    const auto old_dpi = window.dpi;
    window.dpi = owner.dpi;
    window.visible = IsWindowVisible(state.hwnd);
    window.minimized = owner.minimized;
    graphics::update_size(window);
    if (old_width != window.width || old_height != window.height || old_dpi != window.dpi)
    {
        InvalidateRect(state.hwnd, nullptr, FALSE);
    }
}

std::shared_ptr<graphics::canvas> begin_canvas(node& state)
{
    require_idle(state);
    gui::flush_layout(owner_window(state));
    sync_canvas(state);
    return graphics::begin_frame(*state.canvas_window);
}
} // namespace tx_generated::gui
