#include "backend/cpp/native_gui_result.hpp"
#include "backend/cpp/native_gui_abi.hpp"

using namespace tx_generated;
using graphics::resource;

extern "C" int txrt_native_gui_create_text_box(resource* parent, bool multiline,
    const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        const auto value = native_gui::create(native_gui::require_node(parent), tx::graphics_kind::native_text_box, "");
        value->editor->multiline = multiline;
        value->height = {gui::length_mode::fixed, multiline ? 120.0 : 42.0};
        return graphics::make_handle(value);
    });
}

extern "C" int txrt_native_gui_text(resource* control, void** result) noexcept
{
    return native_gui::leaf_call([&]
    {
        const auto& state = native_gui::require_node(control);
        *result = detail::make_handle<std::string>(tx::ui::encode_utf8(state.editor->text()));
    });
}

extern "C" int txrt_native_gui_set_read_only(resource* control, bool read_only) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        if (read_only && state.composing)
        {
            native_gui::owner_window(state).host->cancel_composition();
            state.composing = false;
            state.composition.clear();
        }
        state.editor->read_only = read_only;
        if (native_gui::root_node(state).focused.lock().get() == &state)
        {
            native_gui::owner_window(state).host->enable_ime(!read_only && !state.password);
        }
        native_gui::refresh_editor(state, false);
    });
}

extern "C" int txrt_native_gui_set_password(resource* control, bool password) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        if (password && state.editor->multiline)
        {
            native_gui::fail("invalid_argument", "密码模式只支持单行输入框");
        }
        state.password = password;
        if (native_gui::root_node(state).focused.lock().get() == &state)
        {
            native_gui::owner_window(state).host->enable_ime(!password && !state.editor->read_only);
        }
        native_gui::refresh_editor(state, false);
    });
}

extern "C" int txrt_native_gui_set_text_limit(resource* control, std::int64_t limit) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        if (limit < 1 || limit > 1048576 || std::uint64_t(limit) < state.editor->text().size())
        {
            native_gui::fail("invalid_argument", "文本上限必须为 1–1048576 且不小于当前文本长度");
        }
        state.editor->limit = static_cast<std::size_t>(limit);
    });
}

extern "C" int txrt_native_gui_set_selection(resource* control, std::int64_t start, std::int64_t end) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        if (start < 0 || end < start || std::uint64_t(end) > state.editor->text().size())
        {
            native_gui::fail("invalid_argument", "输入框选区超出 Unicode 标量边界");
        }
        state.editor->select(static_cast<std::size_t>(start), static_cast<std::size_t>(end));
        native_gui::refresh_editor(state, false);
    });
}

extern "C" int txrt_native_gui_selection_start(resource* control, std::int64_t* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        *result = native_gui::require_node(control).editor->selection().first;
    });
}

extern "C" int txrt_native_gui_selection_end(resource* control, std::int64_t* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        *result = native_gui::require_node(control).editor->selection().second;
    });
}
