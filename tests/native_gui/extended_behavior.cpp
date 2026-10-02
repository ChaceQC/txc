#include "stdlib/native_gui/containers.hpp"
#include "stdlib/native_gui/core/image_io.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace tx_generated;
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

template<class operation>
void rejects(operation run)
{
    bool rejected = false;
    try
    {
        run();
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    require(rejected, "invalid operation was not rejected");
}

void key(ui::window& window, const std::string& value, bool shift = false, bool ctrl = false)
{
    tx::ui::window_event input{tx::ui::event_kind::key_down};
    input.key = value;
    input.shift = shift;
    input.ctrl = ctrl;
    ui::process_event(window, input);
}

void pointer(ui::window& window, tx::ui::event_kind kind, double x, double y)
{
    tx::ui::window_event input{kind};
    input.x = x;
    input.y = y;
    input.button = 1;
    ui::process_event(window, input);
}

void verify_models()
{
    tx::ui::data_model list(tx::ui::data_kind::list);
    list.edit({{1, 0, {U"first"}}, {2, 0, {U"second"}}}, tx::ui::data_edit::replace);
    const auto revision = list.revision();
    rejects([&]
    {
        list.edit({{3, 0, {U"a"}}, {3, 0, {U"b"}}}, tx::ui::data_edit::append);
    });
    require(list.revision() == revision && list.snapshot().rows.size() == 2, "failed batch changed snapshot");
    list.reorder({2, 1});
    require(list.snapshot().rows[0].id == 2, "row order did not follow stable IDs");
    list.remove({1});
    rejects([&]
    {
        list.edit({{1, 0, {U"reused"}}}, tx::ui::data_edit::append);
    });
    const auto old_request = list.begin_page(), next_request = list.begin_page();
    rejects([&]
    {
        list.apply_page(old_request, list.revision(), {{3, 0, {U"stale"}}});
    });
    list.apply_page(next_request, list.revision(), {{3, 0, {U"current"}}});
    require(list.snapshot().rows[0].id == 3, "latest page not applied");
    tx::ui::data_model tree(tx::ui::data_kind::tree);
    tree.edit({{1, 0, {U"root"}}, {2, 1, {U"child"}}, {3, 2, {U"leaf"}}}, tx::ui::data_edit::replace);
    rejects([&]
    {
        tree.edit({{1, 3, {U"cycle"}}}, tx::ui::data_edit::update);
    });
    tree.remove({2});
    require(tree.snapshot().rows.size() == 1, "tree removal did not delete descendants");
    tx::ui::data_model table(tx::ui::data_kind::table, {{1, U"A", 100}, {2, U"B", 120}});
    table.edit({{1, 0, {U"first", U"second"}}}, tx::ui::data_edit::replace);
    table.set_column_order({2, 1});
    require(table.column_order()[0] == 1 && table.snapshot().rows[0].cells[0] == U"first", "column reorder changed cell identity");
}

void verify_containers(ui::window& window, ui::node& root)
{
    const auto tabs = ui::create_tabs(root);
    const auto first = ui::add_tab(*tabs, "滚动页"), second = ui::add_tab(*tabs, "分隔页");
    const auto scroll = ui::create_scroll(*first, false, true);
    const auto content = ui::content_panel(*scroll, 0);
    std::vector<std::shared_ptr<ui::node>> buttons;
    for (unsigned index = 0; index < 30; ++index)
    {
        const auto button = ui::create(*content, tx::graphics_kind::native_button, "可聚焦条目");
        button->height = {gui::length_mode::fixed, 42};
        buttons.push_back(button);
    }
    ui::layout(root);
    require(scroll->scroll->height > scroll->scroll->viewport.height && scroll->scroll->vertical_thumb.height > 0,
        "scroll content or thumb missing");
    ui::focus_node(root, buttons.back());
    require(scroll->scroll->y > 0 && buttons.back()->clip.height > 0, "offscreen keyboard focus did not scroll into view");
    const auto old_y = scroll->scroll->y;
    ui::focus_node(root, scroll);
    key(window, "home");
    require(scroll->scroll->y == 0, "scroll Home failed");
    key(window, "end");
    require(scroll->scroll->y >= old_y, "scroll End failed");
    ui::select_tab(*tabs, second->id, false);
    ui::layout(root);
    require(!ui::available(*buttons.front()) && root.focused.lock() == tabs, "hidden page retained focus");
    const auto split = ui::create_split(*second, true, 0.4);
    const auto left = ui::content_panel(*split, 0), right = ui::content_panel(*split, 1);
    ui::layout(root);
    require(left->bounds.x + left->bounds.width <= right->bounds.x, "split panes overlap");
    ui::focus_node(root, split);
    const auto ratio = split->split->ratio;
    key(window, "right");
    require(split->split->ratio > ratio, "split keyboard adjustment failed");
    const auto handle = split->split->handle;
    pointer(window, tx::ui::event_kind::pointer_down, handle.x + 3, handle.y + 10);
    pointer(window, tx::ui::event_kind::pointer_moved, handle.x + 30, handle.y + 10);
    ui::process_event(window, {tx::ui::event_kind::capture_lost});
    const auto canceled_ratio = split->split->ratio;
    pointer(window, tx::ui::event_kind::pointer_moved, handle.x + 100, handle.y + 10);
    require(!split->split->dragging && split->split->ratio == canceled_ratio, "capture loss did not cancel drag");
    ui::close(*left);
    rejects([&]
    {
        ui::content_panel(*split, 0);
    });
    require(ui::content_panel(*split, 1) == right, "closing first pane changed second pane identity");
    key(window, "tab", false, true);
    require(tabs->tabs->selected == first->id, "Ctrl+Tab from nested content failed");
    ui::close(*tabs);
}

void verify_views(ui::app& app, ui::window& window, ui::node& root, const char* bitmap)
{
    const auto table = ui::create_data_view(root, tx::ui::data_kind::table, true,
        {{1, U"编号", 110}, {2, U"十万行虚拟数据 / stable IDs", 680}});
    std::vector<tx::ui::data_row> rows;
    for (std::int64_t id = 1; id <= 100000; ++id)
    {
        rows.push_back({id, 0, {tx::ui::decode_utf8(std::to_string(id)).scalars, U"只为可见行绘制文本"}});
    }
    table->data->model.edit(std::move(rows), tx::ui::data_edit::replace);
    ui::refresh_data(*table);
    ui::layout(root);
    require(table->children.empty() && table->data->last - table->data->first < 30, "virtual view materialized rows");
    table->height = {gui::length_mode::fixed, 2000};
    ui::dirty(*table);
    ui::layout(root);
    require(table->data->last - table->data->first < 30, "virtual rows ignored ancestor clipping");
    table->height = {gui::length_mode::stretch, 1};
    ui::dirty(*table);
    ui::layout(root);
    ui::focus_node(root, table);
    key(window, "end");
    require(table->data->selected.contains(100000) && table->scroll->y > 1000000, "large-data End did not reach last row");
    key(window, "up", true);
    require(table->data->selected.size() == 2, "Shift selection did not retain anchor");
    const auto image = ui::render(root);
    require(table->data->text_cache.size() < 70, "text cache is not bounded to visible rows");
    tx::ui::save_bitmap(image, bitmap);
    app.events.clear();
    pointer(window, tx::ui::event_kind::pointer_down, table->bounds.x + 12, table->bounds.y + 10);
    pointer(window, tx::ui::event_kind::pointer_up, table->bounds.x + 12, table->bounds.y + 10);
    require(std::any_of(app.events.begin(), app.events.end(), [&](const auto& event)
    {
        return event.kind == "sort_requested" && event.column_id == 1 && event.revision == table->data->model.revision();
    }), "header sort request lost column ID or revision");
    ui::close(*table);
    const auto tree = ui::create_data_view(root, tx::ui::data_kind::tree, false);
    tree->data->model.edit({{1, 0, {U"root"}, true}, {2, 1, {U"child"}}, {3, 0, {U"other"}}}, tx::ui::data_edit::replace);
    ui::refresh_data(*tree);
    ui::layout(root);
    require(tree->data->visible.size() == 2, "collapsed tree displayed children");
    ui::focus_node(root, tree);
    key(window, "down");
    key(window, "right");
    ui::layout(root);
    require(tree->data->visible.size() == 3 && tree->data->expanded.contains(1), "tree expansion failed");
    key(window, "right");
    require(tree->data->cursor == 2, "tree Right did not enter child");
    key(window, "left");
    require(tree->data->cursor == 1, "tree Left did not return to parent");
    key(window, "right");
    ui::set_expanded(*tree, 1, false, false);
    require(tree->data->cursor == 1 && tree->data->selected.contains(2), "collapse lost selection identity or kept hidden cursor");
    ui::close(*tree);
}

void verify_nested_scroll(ui::window& window, ui::node& root)
{
    const auto outer = ui::create_scroll(root, false, true);
    const auto content = ui::content_panel(*outer, 0);
    const auto inner = ui::create_scroll(*content, false, true);
    inner->height = {gui::length_mode::fixed, 120};
    const auto inner_content = ui::content_panel(*inner, 0);
    for (unsigned index = 0; index < 10; ++index)
    {
        const auto button = ui::create(*inner_content, tx::graphics_kind::native_button, "嵌套滚动");
        button->height = {gui::length_mode::fixed, 42};
    }
    const auto filler = ui::create(*content, tx::graphics_kind::native_panel, "");
    filler->height = {gui::length_mode::fixed, 800};
    ui::layout(root);
    tx::ui::window_event wheel{tx::ui::event_kind::wheel};
    wheel.x = inner->bounds.x + 20;
    wheel.y = inner->bounds.y + 40;
    wheel.wheel = -1;
    ui::process_event(window, wheel);
    require(inner->scroll->y > 0 && outer->scroll->y == 0, "nested wheel did not prefer inner scroll");
    ui::scroll_to(*inner, 0, 1000000, false);
    ui::process_event(window, wheel);
    require(outer->scroll->y > 0, "wheel did not bubble at inner scroll boundary");
    ui::close(*outer);
}

void verify_visual_edit(ui::window& window, ui::node& root)
{
    const auto input = ui::create(root, tx::graphics_kind::native_text_box, "אבג");
    ui::layout(root);
    ui::focus_node(root, input);
    input->editor->select(0, 0);
    key(window, "left");
    require(input->editor->caret() == 1, "editor left key still follows logical order");
    key(window, "home");
    require(input->editor->caret() == 3, "editor Home is not visual");
    ui::set_text(*input, "éx");
    input->password = true;
    input->text_layout.reset();
    input->editor->select(0, 0);
    key(window, "right");
    require(input->editor->caret() == 2, "password navigation entered a grapheme");
    ui::close(*input);
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
        verify_models();
        detail::current_runtime_context().main_thread = true;
        const auto app = ui::open_app(argv[1]);
        const auto window = ui::create_window(*app, "native GUI extended checks", 640, 480);
        const auto root = ui::root(*window);
        root->padding = 8;
        window->host->show();
        window->visible = true;
        for (unsigned step = 0; step < 4; ++step)
        {
            ui::next_event(*app, 20);
        }
        verify_containers(*window, *root);
        verify_nested_scroll(*window, *root);
        verify_views(*app, *window, *root, argv[2]);
        verify_visual_edit(*window, *root);
        ui::close_app(*app);
        std::cout << "visual editor / scroll / tabs / split / atomic model / virtual table / tree PASS\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
