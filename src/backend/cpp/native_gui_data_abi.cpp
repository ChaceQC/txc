#include "backend/cpp/native_gui_data_bridge.hpp"

using namespace tx_generated;
using graphics::resource;

extern "C" int txrt_native_gui_create_list_view(resource* parent, bool multiple, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::create_data_view(native_gui::require_node(parent), tx::ui::data_kind::list, multiple));
    });
}

extern "C" int txrt_native_gui_create_tree_view(resource* parent, bool multiple, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::create_data_view(native_gui::require_node(parent), tx::ui::data_kind::tree, multiple));
    });
}

extern "C" int txrt_native_gui_create_table_view(resource* parent, const void* value, bool multiple,
    const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        const auto& records = detail::vector_value<std::any>(value).data().values;
        if (records.empty() || records.size() > 256)
        {
            native_gui::fail("invalid_argument", "表格需要 1–256 列");
        }
        std::vector<tx::ui::data_column> columns;
        for (const auto& value : records)
        {
            const auto& record = std::any_cast<const dynamic_struct&>(value);
            tx::ui::data_column column;
            column.id = std::any_cast<std::int64_t>(record->read_field(0));
            column.title = tx::ui::decode_utf8(std::any_cast<std::string>(record->read_field(1))).scalars;
            column.width = std::any_cast<double>(record->read_field(2));
            const auto alignment = std::any_cast<std::string>(record->read_field(3));
            if (alignment != "left" && alignment != "center" && alignment != "right")
            {
                native_gui::fail("invalid_argument", "列对齐必须为 left/center/right");
            }
            column.alignment = alignment == "left" ? 0 : alignment == "center" ? 1 : 2;
            column.visible = std::any_cast<bool>(record->read_field(4));
            columns.push_back(std::move(column));
        }
        return graphics::make_handle(native_gui::create_data_view(native_gui::require_node(parent),
            tx::ui::data_kind::table, multiple, std::move(columns)));
    });
}

#define tx_native_data_edit(kind, operation, edit_kind) \
extern "C" int txrt_native_gui_##operation##_##kind##_view(resource* control, const void* rows) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        auto& state = native_gui::require_node(control); \
        state.data->model.edit(native_gui::read_data_rows<tx::ui::data_kind::kind>(rows), tx::ui::data_edit::edit_kind); \
        native_gui::refresh_data(state); \
    }); \
}

tx_native_data_edit(list, replace_items, replace)
tx_native_data_edit(list, append_items, append)
tx_native_data_edit(list, update_items, update)
tx_native_data_edit(table, replace_items, replace)
tx_native_data_edit(table, append_items, append)
tx_native_data_edit(table, update_items, update)
tx_native_data_edit(tree, replace_items, replace)
tx_native_data_edit(tree, append_items, append)
tx_native_data_edit(tree, update_items, update)
#undef tx_native_data_edit

#define tx_native_data_ids(kind, operation, method) \
extern "C" int txrt_native_gui_##operation##_##kind##_view(resource* control, const void* ids) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        auto& state = native_gui::require_node(control); \
        state.data->model.method(native_gui::read_data_ids(ids)); \
        native_gui::refresh_data(state); \
    }); \
}

tx_native_data_ids(list, remove_items, remove)
tx_native_data_ids(list, set_order, reorder)
tx_native_data_ids(table, remove_items, remove)
tx_native_data_ids(table, set_order, reorder)
tx_native_data_ids(tree, remove_items, remove)
tx_native_data_ids(tree, set_order, reorder)
#undef tx_native_data_ids

#define tx_native_data_page(kind) \
extern "C" int txrt_native_gui_apply_page_##kind##_view(resource* control, std::int64_t request, \
    std::int64_t revision, const void* rows) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        auto& state = native_gui::require_node(control); \
        state.data->model.apply_page(request, revision, native_gui::read_data_rows<tx::ui::data_kind::kind>(rows)); \
        native_gui::refresh_data(state); \
    }); \
}

tx_native_data_page(list)
tx_native_data_page(table)
tx_native_data_page(tree)
#undef tx_native_data_page

extern "C" int txrt_native_gui_set_column_width(resource* control, std::int64_t id, double width) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        state.data->model.set_column_width(id, width);
        native_gui::refresh_data(state);
    });
}

extern "C" int txrt_native_gui_set_column_visible(resource* control, std::int64_t id, bool visible) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        state.data->model.set_column_visible(id, visible);
        native_gui::refresh_data(state);
    });
}

extern "C" int txrt_native_gui_set_column_order(resource* control, const void* ids) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        state.data->model.set_column_order(native_gui::read_data_ids(ids));
        native_gui::refresh_data(state);
    });
}

extern "C" int txrt_native_gui_set_expanded(resource* control, std::int64_t id, bool expanded) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_expanded(native_gui::require_node(control), id, expanded, false);
    });
}

extern "C" int txrt_native_gui_expanded(resource* control, std::int64_t id, bool* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        const auto& state = native_gui::require_node(control);
        if (!state.data->model.snapshot().indices.contains(id))
        {
            native_gui::fail("invalid_argument", "树行 ID 不存在");
        }
        *result = state.data->expanded.contains(id);
    });
}
