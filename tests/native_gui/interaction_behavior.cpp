#include "stdlib/native_gui/combo.hpp"
#include "stdlib/native_gui/core/image_io.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace ui = tx_generated::native_gui;
using tx::ui::event_kind;

namespace
{
void require(bool value, const char* message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}

void key(ui::window& window, const std::string& key, bool alt = false, bool repeat = false)
{
    tx::ui::window_event input{event_kind::key_down};
    input.key = key;
    input.alt = alt;
    input.repeat = repeat;
    ui::process_event(window, input);
}

void pointer(ui::window& window, event_kind kind, double x, double y)
{
    tx::ui::window_event input{kind};
    input.x = x;
    input.y = y;
    input.button = 1;
    ui::process_event(window, input);
}

std::size_t activations(ui::app& app, std::int64_t id)
{
    return std::count_if(app.events.begin(), app.events.end(), [&](const auto& event)
    {
        return event.kind == "activated" && event.source_id == id;
    });
}

void verify(ui::app& app, ui::window& window, ui::node& root, const char* bitmap)
{
    const auto combo = ui::create(root, tx::graphics_kind::native_combo_box, "");
    const auto editor = ui::create(root, tx::graphics_kind::native_text_box, "");
    const auto accept = ui::create(root, tx::graphics_kind::native_button, "确认 (Alt+A)");
    const auto cancel = ui::create(root, tx::graphics_kind::native_button, "取消 (Escape)");
    ui::set_action_button(*accept, false, true);
    ui::set_action_button(*cancel, true, true);
    ui::set_access_key(*accept, "A");
    ui::set_access_key(*combo, "1");
    std::vector<std::u32string> items{U"中文选项", U"English", U"العربية"};
    items.resize(20, U"滚动候选");
    ui::set_combo_items(*combo, items);
    ui::layout(root);
    ui::focus_node(root, combo);
    key(window, "down");
    require(combo->combo->selected == 0, "closed combo navigation failed");
    key(window, "f4");
    key(window, "end");
    require(combo->combo->selected == 0 && combo->combo->candidate == 19, "candidate changed committed selection");
    key(window, "escape");
    require(!combo->combo->open && activations(app, cancel->id) == 0, "escape did not prioritize popup");
    key(window, "f4");
    key(window, "end");
    key(window, "enter");
    require(combo->combo->selected == 19 && activations(app, accept->id) == 0, "combo enter activated default");
    key(window, "f4");
    const auto popup = ui::combo_popup(*combo);
    pointer(window, event_kind::pointer_down, popup.x + 10, popup.y + 10);
    pointer(window, event_kind::pointer_up, popup.x + 10, popup.y + 10);
    require(combo->combo->selected == 12 && !combo->combo->open, "popup hit did not include scroll offset");
    key(window, "f4");
    pointer(window, event_kind::pointer_down, popup.x + 10, popup.y + 42);
    key(window, "escape");
    pointer(window, event_kind::pointer_up, combo->bounds.x + 10, combo->bounds.y + 10);
    require(!combo->combo->open && combo->combo->selected == 12 && root.pressed.expired(),
        "keyboard cancellation retained pointer capture or reopened combo");
    // 外部点击取消，释放到按钮上也不能激活底层控件。
    ui::open_combo(*combo);
    pointer(window, event_kind::pointer_down, 630, 470);
    pointer(window, event_kind::pointer_up, accept->bounds.x + 10, accept->bounds.y + 10);
    require(activations(app, accept->id) == 0, "outside dismissal clicked through");
    key(window, "1", true);
    require(combo->combo->open, "digit access key did not open combo");
    ui::process_event(window, {event_kind::capture_lost});
    require(!combo->combo->open, "capture loss did not cancel popup");
    key(window, "a", true);
    require(activations(app, accept->id) == 1, "access key did not activate");
    key(window, "a", true, true);
    require(activations(app, accept->id) == 1, "access key repeat activated");
    ui::set_access_key(*cancel, "a");
    key(window, "a", true);
    require(root.focused.lock() == cancel && activations(app, cancel->id) == 0, "duplicate access key activated");
    ui::focus_node(root, editor);
    app.events.clear();
    key(window, "enter");
    require(activations(app, accept->id) == 1, "single line did not invoke default");
    require(std::any_of(app.events.begin(), app.events.end(), [](const auto& event)
    {
        return event.kind == "text_committed";
    }), "single line lost existing commit event");
    editor->editor->multiline = true;
    key(window, "enter");
    require(editor->editor->text() == U"\n" && activations(app, accept->id) == 1, "multiline enter invoked default");
    editor->composing = true;
    key(window, "escape");
    key(window, "a", true);
    require(activations(app, cancel->id) == 0 && activations(app, accept->id) == 1, "IME leaked window action");
    editor->composing = false;
    key(window, "escape");
    key(window, "escape", false, true);
    require(activations(app, cancel->id) == 1, "cancel action repeat handling failed");
    ui::set_enabled(*cancel, false);
    key(window, "escape");
    require(activations(app, cancel->id) == 1, "disabled action activated");
    ui::focus_node(root, combo);
    key(window, "f4");
    ui::set_visible(*combo, false);
    require(!combo->combo->open, "hidden popup survived");
    ui::set_visible(*combo, true);
    const auto revision = combo->revision;
    bool rejected = false;
    try
    {
        ui::select_combo(*combo, 100, false);
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    require(rejected && combo->revision == revision, "bad index changed state");
    ui::set_combo_items(*combo, {U"中文 · 选择", U"English · Choice", U"العربية"});
    ui::select_combo(*combo, 0, false);
    ui::focus_node(root, combo);
    ui::open_combo(*combo);
    tx::ui::save_bitmap(ui::render(root), bitmap);
    ui::close(*accept);
    root.focused.reset();
    key(window, "enter");
    require(activations(app, accept->id) == 1, "closed default activated");
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
        const auto window = ui::create_window(*app, "Native GUI interaction checks", 640, 480);
        const auto root = ui::root(*window);
        window->host->show();
        window->visible = true;
        for (int step = 0; step < 4; ++step)
        {
            ui::next_event(*app, 20);
        }
        verify(*app, *window, *root, argv[2]);
        ui::close_app(*app);
        std::cout << "combo / access keys / default / cancel / IME isolation PASS\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
