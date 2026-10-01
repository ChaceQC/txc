#include "backend/cpp/gui_abi.hpp"
#include "backend/cpp/graphics_result.hpp"
#include "stdlib/gui/windows/state.hpp"

using namespace tx_generated;
using namespace tx_generated::gui;

namespace
{

void set_track(node& state, std::int64_t index, const std::string& mode, double value, bool horizontal)
{
    auto& tracks = horizontal ? state.columns : state.rows;
    if (state.layout != layout_mode::grid || index < 0 ||
        static_cast<std::uint64_t>(index) >= tracks.size())
    {
        fail("invalid_layout", "网格轨道索引越界或容器不是 grid");
    }
    tracks[index] = checked_length(mode, value);
    ++state.revision;
    dirty(state);
}

void require_two_state(node& state)
{
    if (state.three_state)
    {
        fail("invalid_argument", "三态复选框请使用 check_state/set_check_state");
    }
}

} // namespace

extern "C" int txrt_gui_root(tx_generated::graphics::resource* window, const char* result_type, void** result) noexcept
{
    return graphics::result_call(result_type, result, [&]() -> std::any
    {
        return graphics::make_handle(gui::root(graphics::require_window(window)));
    });
}

extern "C" int txrt_gui_create_container(tx_generated::graphics::resource* parent, const char* result_type, void** result) noexcept
{
    return graphics::result_call(result_type, result, [&]() -> std::any
    {
        return graphics::make_handle(create(require_node(parent), tx::graphics_kind::container));
    });
}

extern "C" int txrt_gui_create_label(tx_generated::graphics::resource* parent, const void* text, const char* result_type, void** result) noexcept
{
    return graphics::result_call(result_type, result, [&]() -> std::any
    {
        return graphics::make_handle(create(require_node(parent), tx::graphics_kind::label, detail::text_value(text)));
    });
}

extern "C" int txrt_gui_create_button(tx_generated::graphics::resource* parent, const void* text, const char* result_type, void** result) noexcept
{
    return graphics::result_call(result_type, result, [&]() -> std::any
    {
        return graphics::make_handle(create(require_node(parent), tx::graphics_kind::button, detail::text_value(text)));
    });
}

extern "C" int txrt_gui_create_text_box(tx_generated::graphics::resource* parent, bool multiline, const char* result_type, void** result) noexcept
{
    return graphics::result_call(result_type, result, [&]() -> std::any
    {
        return graphics::make_handle(create(require_node(parent), tx::graphics_kind::text_box, {}, multiline));
    });
}

extern "C" int txrt_gui_create_check_box(tx_generated::graphics::resource* parent, const void* text, bool three_state, const char* result_type, void** result) noexcept
{
    return graphics::result_call(result_type, result, [&]() -> std::any
    {
        return graphics::make_handle(create(require_node(parent), tx::graphics_kind::check_box, detail::text_value(text), three_state));
    });
}

extern "C" int txrt_gui_set_column(tx_generated::graphics::resource* parent, double padding, double gap) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_layout(require_node(parent), layout_mode::column, padding, gap);
    });
}

extern "C" int txrt_gui_set_row(tx_generated::graphics::resource* parent, double padding, double gap) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_layout(require_node(parent), layout_mode::row, padding, gap);
    });
}

extern "C" int txrt_gui_set_grid(tx_generated::graphics::resource* parent, std::int64_t rows, std::int64_t columns, double padding, double gap) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_layout(require_node(parent), layout_mode::grid, padding, gap, rows, columns);
    });
}

extern "C" int txrt_gui_set_grid_row(tx_generated::graphics::resource* parent, std::int64_t index, const void* value_mode, double value_value) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_track(require_node(parent), index, detail::text_value(value_mode), value_value, false);
    });
}

extern "C" int txrt_gui_set_grid_column(tx_generated::graphics::resource* parent, std::int64_t index, const void* value_mode, double value_value) noexcept
{
    return detail::invoke_leaf([&]
    {
        set_track(require_node(parent), index, detail::text_value(value_mode), value_value, true);
    });
}

extern "C" int txrt_gui_flush_layout(tx_generated::graphics::resource* window) noexcept
{
    return detail::invoke_leaf([&]
    {
        gui::flush_layout(graphics::require_window(window));
    });
}

extern "C" int txrt_gui_set_default_button(tx_generated::graphics::resource* control) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_node(control);
        auto& root = root_node(state);
        if (const auto old = root.default_button.lock(); old && !old->closed)
        {
            SendMessageW(old->hwnd, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
        }
        root.default_button = state.shared_from_this();
        SendMessageW(state.hwnd, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
    });
}

extern "C" int txrt_gui_set_cancel_button(tx_generated::graphics::resource* control) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_node(control);
        root_node(state).cancel_button = state.shared_from_this();
    });
}

extern "C" int txrt_gui_set_checked(tx_generated::graphics::resource* control, bool checked) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_node(control);
        require_two_state(state);
        set_check(state, checked ? BST_CHECKED : BST_UNCHECKED);
    });
}

extern "C" int txrt_gui_checked(tx_generated::graphics::resource* control, bool* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_node(control);
        require_two_state(state);
        *result = state.check == BST_CHECKED;
    });
}

extern "C" int txrt_gui_set_check_state(tx_generated::graphics::resource* control, const void* state) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& control_state = require_node(control);
        const auto& text = detail::text_value(state);
        if (text != "off" && text != "on" && text != "mixed")
        {
            fail("invalid_argument", "复选框状态必须为 off/on/mixed");
        }
        set_check(control_state, text == "off" ? 0 : text == "on" ? 1 : 2);
    });
}

extern "C" int txrt_gui_check_state(tx_generated::graphics::resource* control, void** result) noexcept
{
    return detail::invoke_leaf([&]
    {
        const auto& state = require_node(control);
        *result = detail::make_handle<std::string>(state.check == 0 ? "off" : state.check == 1 ? "on" : "mixed");
    });
}

extern "C" int txrt_gui_set_read_only(tx_generated::graphics::resource* control, bool read_only) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_node(control);
        if (!SendMessageW(state.hwnd, EM_SETREADONLY, read_only, 0))
        {
            platform_fail(owner_window(state).owner.lock().get(), "设置输入框只读", GetLastError());
        }
        ++state.revision;
    });
}

extern "C" int txrt_gui_set_password(tx_generated::graphics::resource* control, bool password) noexcept
{
    return detail::invoke_leaf([&]
    {
        gui::set_password(require_node(control), password);
    });
}

extern "C" int txrt_gui_set_text_limit(tx_generated::graphics::resource* control, std::int64_t limit) noexcept
{
    return detail::invoke_leaf([&]
    {
        auto& state = require_node(control);
        if (limit < 1 || limit > 16384 || scalar_offsets(wide_text(state.text)).size() - 1 > static_cast<std::size_t>(limit))
        {
            fail("invalid_argument", "输入长度上限必须为 1–16384 且不小于当前文本");
        }
        state.text_limit = limit;
        ++state.revision;
    });
}

extern "C" int txrt_gui_set_selection(tx_generated::graphics::resource* control, std::int64_t start, std::int64_t end) noexcept
{
    return detail::invoke_leaf([&]
    {
        gui::set_selection(require_node(control), start, end);
    });
}

extern "C" int txrt_gui_selection_start(tx_generated::graphics::resource* control, std::int64_t* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = selection(require_node(control), false);
    });
}

extern "C" int txrt_gui_selection_end(tx_generated::graphics::resource* control, std::int64_t* result) noexcept
{
    return detail::invoke_leaf([&]
    {
        *result = selection(require_node(control), true);
    });
}
