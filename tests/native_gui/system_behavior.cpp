#include "stdlib/native_gui/commands.hpp"
#include "stdlib/native_gui/dialogs.hpp"
#include "stdlib/native_gui/theme.hpp"
#include "stdlib/native_gui/accessibility.hpp"
#include "stdlib/native_gui/combo.hpp"
#include "stdlib/native_gui/core/image_io.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <algorithm>
#include <iostream>

namespace ui = tx_generated::native_gui;

namespace
{
void require(bool value, const char* message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}

void key(ui::window& window, const std::string& name, bool ctrl = false)
{
    tx::ui::window_event event{tx::ui::event_kind::key_down};
    event.key = name;
    event.ctrl = ctrl;
    ui::process_event(window, event);
}

void verify(ui::app& app, ui::window& window, ui::node& root, const char* bitmap)
{
    auto action = ui::create_command(window, "执行命令", "ctrl+k", true);
    auto button = ui::create(root, tx::graphics_kind::native_button, "");
    auto second = ui::create(root, tx::graphics_kind::native_button, "");
    ui::bind_command(*button, *action);
    ui::bind_command(*second, *action);
    bool rejected = false;
    try
    {
        ui::create_command(window, "冲突", "ctrl+k", false);
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    require(rejected, "duplicate shortcut accepted");
    app.events.clear();
    key(window, "k", true);
    require(action->checked && button->checked && second->checked, "shared command state did not synchronize");
    require(app.events.size() == 1 && app.events.front().kind == "command", "command dispatched more than once");
    action->enabled = false;
    ui::update_command(*action);
    ui::activate(*button);
    require(app.events.size() == 1 && !ui::available(*second), "disabled binding activated");
    action->enabled = true;
    ui::update_command(*action);
    auto menu = ui::create_menu(window);
    ui::append_menu(*menu, "", action, {});
    auto bar = ui::create_menu(window);
    ui::append_menu(*bar, "文件", {}, menu);
    window.menu_bar = bar;
    ui::dirty(root);
    ui::focus_node(root, button);
    key(window, "f10");
    require(bool(window.menu_popup), "F10 did not open menu");
    key(window, "down");
    key(window, "enter");
    require(!window.menu_popup && !action->checked && root.focused.lock() == button, "menu execution/focus failed");
    ui::show_menu(*menu, 20, 80);
    key(window, "escape");
    require(!window.menu_popup, "menu escape failed");
    auto editor = ui::create(root, tx::graphics_kind::native_text_box, "中文 A😀B");
    ui::set_accessibility(*editor, "输入内容", "编辑文本");
    ui::layout(root);
    const auto editor_id = "n" + std::to_string(editor->id);
    ui::accessible_action set{editor_id, "value"};
    set.text = U"读屏修改😀";
    require(window.accessibility->action(set) && editor->editor->text() == set.text, "accessible value failed");
    set.operation = "text_selection";
    set.start = 0;
    set.end = 2;
    require(window.accessibility->action(set), "accessible text selection failed");
    require(!window.accessibility->text_bounds(editor_id, 0, 2).empty(), "text bounds missing");
    editor->password = true;
    const auto password = window.accessibility->tree().at(editor_id);
    require(password.text.empty() && !password.text_capable && !window.accessibility->action(set), "password exposed via accessibility");
    editor->password = false;
    auto slider = ui::create(root, tx::graphics_kind::native_slider, "");
    const auto slider_id = "n" + std::to_string(slider->id);
    ui::accessible_action range{slider_id, "range"};
    range.value = 37;
    require(window.accessibility->action(range) && slider->value == 37, "range action failed");
    range.value = 101;
    require(!window.accessibility->action(range) && slider->value == 37, "range accepted invalid value");
    auto combo = ui::create(root, tx::graphics_kind::native_combo_box, "");
    ui::set_combo_items(*combo, {U"第一项", U"第二项"});
    const auto combo_id = "n" + std::to_string(combo->id);
    const auto before_items = window.accessibility->tree().at(combo_id).children;
    require(window.accessibility->action({before_items[1], "select"}) && combo->combo->selected == 1,
        "accessible selection failed");
    ui::set_combo_items(*combo, {U"新项目"});
    require(!window.accessibility->action({before_items[0], "select"}), "stale combo item selected replacement");
    auto dialog = ui::create_window(app, "模态检查", 320, 180);
    ui::root(*dialog);
    ui::show_modal(*dialog, window);
    require(!window.accessibility->tree().at("n" + std::to_string(button->id)).enabled, "modal owner still accessible as enabled");
    const auto checked = action->checked;
    key(window, "k", true);
    require(action->checked == checked, "modal owner shortcut executed");
    ui::end_dialog(*dialog, 42);
    require(window.modal_child.expired() && dialog->dialog_result == 42 && root.focused.lock() == button, "modal restoration failed");
    ui::set_theme(root, "high_contrast");
    ui::set_font_size(root, 18);
    ui::set_ui_scale(root, 1.25);
    ui::layout(root);
    require(window.dpi == window.host->scale() * 120, "application scale mismatch");
    tx::ui::save_bitmap(ui::render(root), bitmap);
    ui::close(*editor);
    require(!window.accessibility->tree().contains(editor_id), "closed element remained accessible");
    ui::close_command(*action);
    require(!ui::available(*button), "closed command remained actionable");
}
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        return 2;
    }
    try
    {
        tx_generated::detail::current_runtime_context().main_thread = true;
        const auto app = ui::open_app(argv[1]);
        const auto window = ui::create_window(*app, "TX system checks", 720, 620);
        const auto root = ui::root(*window);
        window->host->show();
        window->visible = true;
        for (int count = 0; count < 3; ++count)
        {
            ui::next_event(*app, 20);
        }
        verify(*app, *window, *root, argv[2]);
        ui::close_app(*app);
        std::cout << "commands / menus / modality / theme / accessibility behavior PASS\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
