#include "backend/cpp/gui_abi.hpp"
#include "backend/cpp/graphics_result.hpp"
#include "stdlib/gui/windows/state.hpp"
#include "stdlib/gui/windows/commands.hpp"

using namespace tx_generated;
using namespace tx_generated::gui;

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_ID(type) \
extern "C" int txrt_gui_id_##type(tx_generated::graphics::resource* control, std::int64_t* result) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        *result = require_node(control).id; \
    }); \
}

TX_GUI_ID(container)
TX_GUI_ID(label)
TX_GUI_ID(button)
TX_GUI_ID(text_box)
TX_GUI_ID(check_box)
TX_GUI_ID(list_view)
TX_GUI_ID(table_view)
TX_GUI_ID(tree_view)
#undef TX_GUI_ID

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_IS_OPEN(type) \
extern "C" int txrt_gui_is_open_##type(tx_generated::graphics::resource* control, bool* result) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        *result = !require_node(control, true).closed && static_cast<node*>(control)->hwnd; \
    }); \
}

TX_GUI_IS_OPEN(container)
TX_GUI_IS_OPEN(label)
TX_GUI_IS_OPEN(button)
TX_GUI_IS_OPEN(text_box)
TX_GUI_IS_OPEN(check_box)
TX_GUI_IS_OPEN(list_view)
TX_GUI_IS_OPEN(table_view)
TX_GUI_IS_OPEN(tree_view)
#undef TX_GUI_IS_OPEN

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_CLOSE(type) \
extern "C" int txrt_gui_close_##type(tx_generated::graphics::resource* control) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        gui::close(require_node(control, true)); \
    }); \
}

TX_GUI_CLOSE(container)
TX_GUI_CLOSE(label)
TX_GUI_CLOSE(button)
TX_GUI_CLOSE(text_box)
TX_GUI_CLOSE(check_box)
TX_GUI_CLOSE(list_view)
TX_GUI_CLOSE(table_view)
TX_GUI_CLOSE(tree_view)
#undef TX_GUI_CLOSE

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_SET_ENABLED(type) \
extern "C" int txrt_gui_set_enabled_##type(tx_generated::graphics::resource* control, bool enabled) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        auto& state = require_node(control); \
        if (state.bound_command) \
        { \
            auto& command = require_command(state.bound_command.get()); \
            set_command(command, command.text, enabled, command.checked); \
        } \
        else \
        { \
            gui::set_enabled(state, enabled); \
        } \
    }); \
}

TX_GUI_SET_ENABLED(container)
TX_GUI_SET_ENABLED(label)
TX_GUI_SET_ENABLED(button)
TX_GUI_SET_ENABLED(text_box)
TX_GUI_SET_ENABLED(check_box)
TX_GUI_SET_ENABLED(list_view)
TX_GUI_SET_ENABLED(table_view)
TX_GUI_SET_ENABLED(tree_view)
#undef TX_GUI_SET_ENABLED

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_SET_VISIBLE(type) \
extern "C" int txrt_gui_set_visible_##type(tx_generated::graphics::resource* control, bool visible) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        gui::set_visible(require_node(control), visible); \
    }); \
}

TX_GUI_SET_VISIBLE(container)
TX_GUI_SET_VISIBLE(label)
TX_GUI_SET_VISIBLE(button)
TX_GUI_SET_VISIBLE(text_box)
TX_GUI_SET_VISIBLE(check_box)
TX_GUI_SET_VISIBLE(list_view)
TX_GUI_SET_VISIBLE(table_view)
TX_GUI_SET_VISIBLE(tree_view)
#undef TX_GUI_SET_VISIBLE

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_SET_RESERVED_SPACE(type) \
extern "C" int txrt_gui_set_reserved_space_##type(tx_generated::graphics::resource* control, bool reserved) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        auto& state = require_node(control); \
        state.reserved = reserved; \
        ++state.revision; \
        dirty(state); \
    }); \
}

TX_GUI_SET_RESERVED_SPACE(container)
TX_GUI_SET_RESERVED_SPACE(label)
TX_GUI_SET_RESERVED_SPACE(button)
TX_GUI_SET_RESERVED_SPACE(text_box)
TX_GUI_SET_RESERVED_SPACE(check_box)
#undef TX_GUI_SET_RESERVED_SPACE

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_SET_WIDTH(type) \
extern "C" int txrt_gui_set_width_##type(tx_generated::graphics::resource* control, const void* value_mode, double value_value) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        auto& state = require_node(control); \
        state.width = checked_length(detail::text_value(value_mode), value_value); \
        state.explicit_width = true; \
        ++state.revision; \
        dirty(state); \
    }); \
}

TX_GUI_SET_WIDTH(container)
TX_GUI_SET_WIDTH(label)
TX_GUI_SET_WIDTH(button)
TX_GUI_SET_WIDTH(text_box)
TX_GUI_SET_WIDTH(check_box)
TX_GUI_SET_WIDTH(list_view)
TX_GUI_SET_WIDTH(table_view)
TX_GUI_SET_WIDTH(tree_view)
#undef TX_GUI_SET_WIDTH

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_SET_HEIGHT(type) \
extern "C" int txrt_gui_set_height_##type(tx_generated::graphics::resource* control, const void* value_mode, double value_value) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        auto& state = require_node(control); \
        state.height = checked_length(detail::text_value(value_mode), value_value); \
        ++state.revision; \
        dirty(state); \
    }); \
}

TX_GUI_SET_HEIGHT(container)
TX_GUI_SET_HEIGHT(label)
TX_GUI_SET_HEIGHT(button)
TX_GUI_SET_HEIGHT(text_box)
TX_GUI_SET_HEIGHT(check_box)
TX_GUI_SET_HEIGHT(list_view)
TX_GUI_SET_HEIGHT(table_view)
TX_GUI_SET_HEIGHT(tree_view)
#undef TX_GUI_SET_HEIGHT

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_SET_CONSTRAINTS(type) \
extern "C" int txrt_gui_set_constraints_##type(tx_generated::graphics::resource* control, double min_width, double min_height, double max_width, double max_height) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        auto& state = require_node(control); \
        checked_dimension(min_width); \
        checked_dimension(min_height); \
        checked_dimension(max_width); \
        checked_dimension(max_height); \
        if (max_width < min_width || max_height < min_height) \
        { \
            fail("invalid_layout", "最大尺寸不能小于最小尺寸"); \
        } \
        state.minimum = {min_width, min_height}; \
        state.maximum = {max_width, max_height}; \
        ++state.revision; \
        dirty(state); \
    }); \
}

TX_GUI_SET_CONSTRAINTS(container)
TX_GUI_SET_CONSTRAINTS(label)
TX_GUI_SET_CONSTRAINTS(button)
TX_GUI_SET_CONSTRAINTS(text_box)
TX_GUI_SET_CONSTRAINTS(check_box)
#undef TX_GUI_SET_CONSTRAINTS

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_SET_MARGIN(type) \
extern "C" int txrt_gui_set_margin_##type(tx_generated::graphics::resource* control, double margin) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        auto& state = require_node(control); \
        state.margin = checked_dimension(margin); \
        ++state.revision; \
        dirty(state); \
    }); \
}

TX_GUI_SET_MARGIN(container)
TX_GUI_SET_MARGIN(label)
TX_GUI_SET_MARGIN(button)
TX_GUI_SET_MARGIN(text_box)
TX_GUI_SET_MARGIN(check_box)
#undef TX_GUI_SET_MARGIN

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_SET_ALIGNMENT(type) \
extern "C" int txrt_gui_set_alignment_##type(tx_generated::graphics::resource* control, const void* horizontal, const void* vertical) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        auto& state = require_node(control); \
        const auto x = checked_alignment(detail::text_value(horizontal)); \
        const auto y = checked_alignment(detail::text_value(vertical)); \
        state.horizontal = x; \
        state.vertical = y; \
        ++state.revision; \
        dirty(state); \
    }); \
}

TX_GUI_SET_ALIGNMENT(container)
TX_GUI_SET_ALIGNMENT(label)
TX_GUI_SET_ALIGNMENT(button)
TX_GUI_SET_ALIGNMENT(text_box)
TX_GUI_SET_ALIGNMENT(check_box)
#undef TX_GUI_SET_ALIGNMENT

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_SET_CELL(type) \
extern "C" int txrt_gui_set_cell_##type(tx_generated::graphics::resource* control, std::int64_t row, std::int64_t column, std::int64_t row_span, std::int64_t column_span) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        gui::set_cell(require_node(control), row, column, row_span, column_span); \
    }); \
}

TX_GUI_SET_CELL(container)
TX_GUI_SET_CELL(label)
TX_GUI_SET_CELL(button)
TX_GUI_SET_CELL(text_box)
TX_GUI_SET_CELL(check_box)
TX_GUI_SET_CELL(list_view)
TX_GUI_SET_CELL(table_view)
TX_GUI_SET_CELL(tree_view)
#undef TX_GUI_SET_CELL

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_SET_TEXT(type) \
extern "C" int txrt_gui_set_text_##type(tx_generated::graphics::resource* control, const void* text) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        auto& state = require_node(control); \
        if (state.bound_command) \
        { \
            auto& command = require_command(state.bound_command.get()); \
            set_command(command, detail::text_value(text), command.enabled, command.checked); \
        } \
        else \
        { \
            gui::set_text(state, detail::text_value(text)); \
        } \
    }); \
}

TX_GUI_SET_TEXT(label)
TX_GUI_SET_TEXT(button)
TX_GUI_SET_TEXT(text_box)
TX_GUI_SET_TEXT(check_box)
#undef TX_GUI_SET_TEXT

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_TEXT(type) \
extern "C" int txrt_gui_text_##type(tx_generated::graphics::resource* control, void** result) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        *result = detail::make_handle<std::string>(require_node(control).text); \
    }); \
}

TX_GUI_TEXT(label)
TX_GUI_TEXT(button)
TX_GUI_TEXT(text_box)
TX_GUI_TEXT(check_box)
#undef TX_GUI_TEXT

// 相同节点操作导出有限的具体类型符号；不存在运行时类型分派。
#define TX_GUI_FOCUS(type) \
extern "C" int txrt_gui_focus_##type(tx_generated::graphics::resource* control, const char* result_type, void** result) noexcept \
{ \
    return graphics::result_call(result_type, result, [&]() -> std::any \
    { \
        gui::focus(require_node(control)); \
        return {}; \
    }); \
}

TX_GUI_FOCUS(button)
TX_GUI_FOCUS(text_box)
TX_GUI_FOCUS(check_box)
TX_GUI_FOCUS(list_view)
TX_GUI_FOCUS(table_view)
TX_GUI_FOCUS(tree_view)
#undef TX_GUI_FOCUS


