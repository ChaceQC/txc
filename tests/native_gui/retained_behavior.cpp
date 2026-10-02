#include "stdlib/native_gui/state.hpp"
#include "stdlib/native_gui/core/image_io.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace tx_generated;
namespace ui = tx_generated::native_gui;

namespace
{
void mouse(ui::window& window, tx::ui::event_kind kind, double x, double y)
{
    tx::ui::window_event event{kind};
    event.x = x;
    event.y = y;
    event.button = 1;
    ui::process_event(window, event);
}

std::size_t actions(const ui::app& app, std::int64_t id)
{
    return std::count_if(app.events.begin(), app.events.end(), [&](const auto& item)
    {
        return item.source_id == id;
    });
}

void click(ui::window& window, const ui::node& control)
{
    const auto x = control.bounds.x + 8, y = control.bounds.y + control.bounds.height / 2;
    mouse(window, tx::ui::event_kind::pointer_down, x, y);
    mouse(window, tx::ui::event_kind::pointer_up, x, y);
}

void key(ui::window& window, const std::string& name, bool down, bool repeat = false)
{
    tx::ui::window_event event{down ? tx::ui::event_kind::key_down : tx::ui::event_kind::key_up};
    event.key = name;
    event.repeat = repeat;
    ui::process_event(window, event);
}

void verify_input(ui::app& app, ui::window& window, ui::node& root, ui::node& button, ui::node& check)
{
    app.events.clear();
    click(window, button);
    assert(actions(app, button.id) == 1);
    mouse(window, tx::ui::event_kind::pointer_down, button.bounds.x + 8, button.bounds.y + 8);
    mouse(window, tx::ui::event_kind::pointer_moved, -10, -10);
    mouse(window, tx::ui::event_kind::pointer_up, -10, -10);
    assert(actions(app, button.id) == 1 && root.pressed.expired());
    click(window, check);
    assert(check.checked && actions(app, check.id) == 1);
    ui::set_enabled(button, false);
    click(window, button);
    assert(actions(app, button.id) == 1);
    key(window, "tab", true);
    assert(root.focused.lock().get() == &check);
    key(window, "space", true);
    key(window, "space", true, true);
    key(window, "space", false);
    assert(!check.checked && actions(app, check.id) == 2);
    key(window, "enter", true);
    key(window, "enter", true, true);
    assert(check.checked && actions(app, check.id) == 3);
    key(window, "space", true);
    ui::process_event(window, {tx::ui::event_kind::focus_lost});
    key(window, "space", false);
    assert(actions(app, check.id) == 3 && root.pressed.expired());
    ui::set_enabled(button, true);
}

void verify_editor(ui::app& app, ui::window& window, ui::node& root)
{
    const auto input = ui::create(root, tx::graphics_kind::native_text_box, "");
    input->editor->multiline = true;
    input->height = {gui::length_mode::fixed, 120};
    ui::layout(root);
    ui::process_event(window, {tx::ui::event_kind::focus_gained});
    ui::focus_node(root, input);
    tx::ui::window_event text{tx::ui::event_kind::text_input};
    text.text = U"é👨‍👩‍👧‍👦";
    ui::process_event(window, text);
    assert(input->editor->text() == text.text);
    key(window, "backspace", true);
    assert(input->editor->text() == U"é");
    tx::ui::window_event undo{tx::ui::event_kind::key_down};
    undo.key = "z";
    undo.ctrl = true;
    ui::process_event(window, undo);
    assert(input->editor->text() == text.text);
    input->password = true;
    app.events.clear();
    text.text = U"test";
    ui::process_event(window, text);
    const auto changed = std::find_if(app.events.begin(), app.events.end(), [](const auto& event)
    {
        return event.kind == "text_changed";
    });
    assert(changed != app.events.end() && changed->text.empty());
    input->password = false;
    ui::set_text(*input, "甲乙");
    input->editor->select(1, 1);
    tx::ui::window_event composition{tx::ui::event_kind::composition};
    composition.composing = true;
    composition.text = U"拼音";
    ui::process_event(window, composition);
    assert(input->editor->text() == U"甲乙");
    assert(ui::display_text(*input) == U"甲拼音乙");
    text.text = U"中";
    ui::process_event(window, text);
    assert(input->editor->text() == U"甲中乙" && !input->composing);
    ui::close(*input);
}
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        return 2;
    }
    detail::current_runtime_context().main_thread = true;
    const auto app = ui::open_app(argv[1]);
    const auto app_lease = graphics::make_handle(app);
    const auto window = ui::create_window(*app, "TX retained GUI checks", 640, 400);
    const auto window_lease = graphics::make_handle(window);
    const auto root = ui::root(*window);
    const auto label = ui::create(*root, tx::graphics_kind::native_label, "自绘 GUI / 中文");
    const auto button = ui::create(*root, tx::graphics_kind::native_button, "点击按钮");
    const auto check = ui::create(*root, tx::graphics_kind::native_check_box, "复选框");
    window->host->show();
    window->visible = true;
    for (unsigned count = 0; count < 10; ++count)
    {
        ui::next_event(*app, 20);
    }
    ui::layout(*root);
    verify_input(*app, *window, *root, *button, *check);
    const auto previous_y = check->bounds.y;
    ui::set_visible(*button, false);
    ui::layout(*root);
    assert(check->bounds.y < previous_y && button->clip.width == 0);
    ui::set_visible(*button, true);
    verify_editor(*app, *window, *root);
    tx::ui::save_bitmap(ui::render(*root), argv[2]);
    ui::close(*root);
    assert(button->closed && check->closed && !window->native_gui_root);
    ui::close_window(*window);
    ui::close_app(*app);
    std::cout << "retained controls / input / layout / rendering / lifetime PASS\n";
}
