#include "backend/cpp/native_gui_data_bridge.hpp"

using namespace tx_generated;
using graphics::resource;

namespace
{
bool displayed(const native_gui::node& state)
{
    for (const auto* current = &state; current;)
    {
        if (!current->visible || !current->layout_visible || current->closed)
        {
            return false;
        }
        const auto parent = current->parent.lock();
        current = parent.get();
    }
    return true;
}
}

#define tx_native_data_query(kind, operation, expression) \
extern "C" int txrt_native_gui_##operation##_##kind##_view(resource* control, std::int64_t* result) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        auto& model = native_gui::require_node(control).data->model; \
        *result = expression; \
    }); \
}

tx_native_data_query(list, revision, model.revision())
tx_native_data_query(table, revision, model.revision())
tx_native_data_query(tree, revision, model.revision())
tx_native_data_query(list, count, model.snapshot().rows.size())
tx_native_data_query(table, count, model.snapshot().rows.size())
tx_native_data_query(tree, count, model.snapshot().rows.size())
tx_native_data_query(list, begin_page, model.begin_page())
tx_native_data_query(table, begin_page, model.begin_page())
tx_native_data_query(tree, begin_page, model.begin_page())
#undef tx_native_data_query

#define tx_native_data_selection(kind) \
extern "C" int txrt_native_gui_selected_ids_##kind##_view(resource* control, void** result) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        const auto& selected = native_gui::require_node(control).data->selected; \
        native_gui::return_data_ids({selected.begin(), selected.end()}, result); \
    }); \
} \
extern "C" int txrt_native_gui_set_selected_ids_##kind##_view(resource* control, const void* ids) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        auto& state = native_gui::require_node(control); \
        native_gui::set_data_selection(state, native_gui::read_data_ids(ids), false); \
        native_gui::reveal_data_cursor(state); \
    }); \
} \
extern "C" int txrt_native_gui_visible_ids_##kind##_view(resource* control, void** result) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        auto& state = native_gui::require_node(control); \
        native_gui::layout(state); \
        const auto& data = *state.data; \
        std::vector<std::int64_t> ids; \
        if (displayed(state) && state.clip.width > 0 && state.clip.height > 0) \
        { \
            for (auto index = data.first; index < data.last; ++index) \
            { \
                ids.push_back(data.model.snapshot().rows[data.visible[index].index].id); \
            } \
        } \
        native_gui::return_data_ids(std::move(ids), result); \
    }); \
} \
extern "C" int txrt_native_gui_set_row_height_##kind##_view(resource* control, double height) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        auto& state = native_gui::require_node(control); \
        if (!std::isfinite(height) || height < 16 || height > 256) \
        { \
            native_gui::fail("invalid_argument", "数据行高必须位于 [16,256]"); \
        } \
        state.data->row_height = height; \
        native_gui::dirty(state); \
    }); \
}

tx_native_data_selection(list)
tx_native_data_selection(table)
tx_native_data_selection(tree)
#undef tx_native_data_selection

namespace
{
void item_text(resource* control, std::int64_t id, void** result)
{
    const auto& snapshot = native_gui::require_node(control).data->model.snapshot();
    const auto found = snapshot.indices.find(id);
    if (found == snapshot.indices.end())
    {
        native_gui::fail("invalid_argument", "行 ID 不存在");
    }
    *result = detail::make_handle<std::string>(tx::ui::encode_utf8(snapshot.rows[found->second].cells[0]));
}
}

extern "C" int txrt_native_gui_item_text_list_view(resource* control, std::int64_t id, void** result) noexcept
{
    return native_gui::leaf_call([&]
    {
        item_text(control, id, result);
    });
}

extern "C" int txrt_native_gui_item_text_tree_view(resource* control, std::int64_t id, void** result) noexcept
{
    return native_gui::leaf_call([&]
    {
        item_text(control, id, result);
    });
}

extern "C" int txrt_native_gui_cell_text(resource* control, std::int64_t id, std::int64_t column_id, void** result) noexcept
{
    return native_gui::leaf_call([&]
    {
        const auto& model = native_gui::require_node(control).data->model;
        const auto found = model.snapshot().indices.find(id);
        if (found == model.snapshot().indices.end())
        {
            native_gui::fail("invalid_argument", "行 ID 不存在");
        }
        for (std::size_t index = 0; index < model.columns().size(); ++index)
        {
            if (model.columns()[index].id == column_id)
            {
                *result = detail::make_handle<std::string>(tx::ui::encode_utf8(model.snapshot().rows[found->second].cells[index]));
                return;
            }
        }
        native_gui::fail("invalid_argument", "列 ID 不存在");
    });
}
