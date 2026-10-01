#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#include "stdlib/gui/windows/models.hpp"
#include "stdlib/gui/windows/dialogs.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <cassert>
#include <iostream>

using namespace tx_generated;
namespace gfx = tx_generated::graphics;
namespace gui = tx_generated::gui;

namespace
{

template<class operation>
void expect(const char* code, operation&& run)
{
    try
    {
        run();
        throw std::runtime_error("expected error");
    }
    catch (const runtime_failure& error)
    {
        assert(error.error().code == code);
    }
}

void input(gfx::app& app, gfx::window& window)
{
    app.events.clear();
    SendMessageW(window.hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(10, 20));
    SendMessageW(window.hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(15, 25));
    SendMessageW(window.hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(15, 25));
    SendMessageW(window.hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(20, 30));
    assert(app.events.size() == 3);
    assert(app.events[0].pointer->x == 15 * 96.0 / window.dpi);
    assert(app.events[1].kind == "pointer_down");
    SendMessageW(window.hwnd, WM_KEYDOWN, 'A', 1 | (0x1e << 16));
    assert(window.keys['A']);
    SendMessageW(window.hwnd, WM_KILLFOCUS, 0, 0);
    assert(!window.keys['A']);
    app.events.clear();
    window.text_input = true;
    SendMessageW(window.hwnd, WM_CHAR, 0xd83d, 0);
    assert(app.events.empty());
    SendMessageW(window.hwnd, WM_CHAR, 0xde00, 0);
    assert(app.events.size() == 1 && app.events.back().text->text == "😀");
    window.keys['A'] = true;
    app.events.clear();
    for (std::size_t index = 0; index <= gfx::event_limit; ++index)
    {
        gfx::event event;
        event.kind = "key_up";
        gfx::enqueue_event(window, std::move(event));
    }
    assert(app.queue_failed && !window.keys['A']);
    app.events.clear();
    app.queue_failed = false;
}

void offscreen(gfx::app& app)
{
    auto surface = gfx::create_surface(app, 8, 8, 96);
    auto handle = gfx::make_handle(surface);
    auto frame = gfx::begin_surface_frame(*surface);
    gfx::save_state(*frame);
    gfx::clip_rectangle(*frame, {0, 0, 1, 1});
    gfx::clear_canvas(*frame, {1, 0, 0, 0.5});
    gfx::restore_state(*frame);
    gfx::end_frame(*frame);
    auto image = gfx::snapshot(*surface);
    const auto rgba = gfx::straight_rgba(*image);
    for (std::size_t index = 0; index < rgba.size(); index += 4)
    {
        assert(rgba[index] >= 253 && rgba[index + 1] == 0 && rgba[index + 2] == 0);
        assert(rgba[index + 3] >= 127 && rgba[index + 3] <= 128);
    }
    auto cancelled = gfx::begin_surface_frame(*surface);
    gfx::clear_canvas(*cancelled, {0, 0, 0, 0});
    gfx::cancel_canvas(*cancelled);
    app.frame.reset();
    assert(gfx::snapshot(*surface)->pixels == image->pixels);
    gfx::close_owned(*surface);
}

void commands(gfx::app& app, gfx::window& window, gui::node& root)
{
    auto command = gui::create_command(window, "保存");
    auto button = gui::create(root, tx::graphics_kind::button, "按钮");
    gui::bind_button(*button, *command);
    auto menu = gui::create_menu(window);
    auto popup = gui::create_menu(window, menu.get(), "文件");
    gui::add_command(*popup, *command);
    gui::set_command(*command, "同步", false, true);
    assert(!IsWindowEnabled(button->hwnd));
    assert((GetMenuState(popup->handle, command->native_id, MF_BYCOMMAND) & MF_GRAYED) != 0);
    gui::set_command(*command, "同步", true, true);
    gui::flush_layout(window);
    gfx::show_window(window);
    app.events.clear();
    SendMessageW(button->hwnd, BM_CLICK, 0, 0);
    bool activated = false;
    for (const auto& event : app.events)
    {
        activated = activated || (event.control && event.control->action == "activated" &&
            event.control->command_id == command->id);
    }
    assert(activated);
    auto modal = gfx::create_window(app, "模态恢复", 100, 80, true);
    gui::show_modal(*modal, window);
    assert(!IsWindowEnabled(window.hwnd));
    gfx::close_window(*modal);
    assert(IsWindowEnabled(window.hwnd));
    EnableWindow(window.hwnd, FALSE);
    auto second = gfx::create_window(app, "原状态保留", 100, 80, true);
    gui::show_modal(*second, window);
    gfx::close_window(*second);
    assert(!IsWindowEnabled(window.hwnd));
    EnableWindow(window.hwnd, TRUE);
}

void data(gfx::app& app, gfx::window& window, gui::node& root)
{
    auto model = gui::create_model(window, tx::graphics_kind::table_model,
        {{7, L"列", 120, LVCFMT_LEFT}});
    gui::edit_model(*model, {{10, {}, {L"甲"}}, {20, {}, {L"乙"}}}, gui::model_edit::replace);
    auto view = gui::create_view(root, *model, tx::graphics_kind::table_view, true);
    gui::set_selected_ids(*view, {20});
    gui::set_order(*model, {20, 10});
    assert(ListView_GetNextItem(view->hwnd, -1, LVNI_SELECTED) == 0);
    NMLVDISPINFOW info{};
    info.hdr.hwndFrom = view->hwnd;
    info.hdr.code = LVN_GETDISPINFOW;
    info.item.mask = LVIF_TEXT;
    info.item.iItem = 0;
    info.item.iSubItem = 0;
    SendMessageW(root.hwnd, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&info));
    assert(std::wstring(info.item.pszText) == L"乙");
    const auto revision = model->revision;
    expect("duplicate_id", [&]
    {
        gui::edit_model(*model, {{30, {}, {L"a"}}, {30, {}, {L"b"}}}, gui::model_edit::append);
    });
    assert(model->revision == revision && model->snapshot->items.size() == 2);
    const auto old_request = gui::begin_page(*model);
    gui::begin_page(*model);
    expect("stale_revision", [&]
    {
        gui::apply_page(*model, old_request, revision, {});
    });
    app.events.clear();
    gui::remove_items(*model, {20});
    assert(view->selection.empty() && app.events.size() == 1);
    auto tree = gui::create_model(window, tx::graphics_kind::tree_model);
    gui::edit_model(*tree, {{1, {}, {L"目录"}, "unloaded"}}, gui::model_edit::replace);
    auto tree_view = gui::create_view(root, *tree, tx::graphics_kind::tree_view, false);
    app.events.clear();
    TreeView_Expand(tree_view->hwnd, tree_view->tree_handles.at(1), TVE_EXPAND);
    assert(tree->snapshot->items[0].load_state == "loading");
    assert(app.events.back().control->action == "expand_requested");
    assert(app.events.back().control->item_id == 1);
    expect("invalid_argument", [&]
    {
        gui::edit_model(*tree, {{1, 2, {L"a"}}, {2, 1, {L"b"}}}, gui::model_edit::replace);
    });
}

} // namespace

int main()
{
    detail::current_runtime_context().main_thread = true;
    const auto app = gfx::open_app();
    const auto app_handle = gfx::make_handle(app);
    const auto window = gfx::create_window(*app, "G2-U3 原生验证", 480, 320, true);
    const auto window_handle = gfx::make_handle(window);
    input(*app, *window);
    offscreen(*app);
    const auto root = gui::root(*window);
    commands(*app, *window, *root);
    data(*app, *window, *root);
    gfx::show_window(*window);
    auto frame = gfx::begin_frame(*window);
    gfx::end_frame(*frame);
    window->target.reset();
    frame = gfx::begin_frame(*window);
    gfx::end_frame(*frame);
    assert(window->target);
    const auto hwnd = window->hwnd;
    gfx::close_app(*app);
    assert(!IsWindow(hwnd) && root->closed && app->image_bytes == 0);
    std::cout << "native input/pixels/commands/models/recreation: PASS\n";
}
