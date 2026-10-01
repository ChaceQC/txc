#include "backend/cpp/gui_data_bridge.hpp"

using namespace tx_generated;
using namespace tx_generated::gui;

#define TX_MODEL_QUERY(kind, operation, expression) \
extern "C" int txrt_gui_data_##operation##_##kind##_model(graphics::resource* value, std::int64_t* result) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        auto& model = require_model(value); \
        *result = expression; \
    }); \
}

TX_MODEL_QUERY(list, revision, model.revision)
TX_MODEL_QUERY(table, revision, model.revision)
TX_MODEL_QUERY(tree, revision, model.revision)
TX_MODEL_QUERY(list, count, static_cast<std::int64_t>(model.snapshot->items.size()))
TX_MODEL_QUERY(table, count, static_cast<std::int64_t>(model.snapshot->items.size()))
TX_MODEL_QUERY(tree, count, static_cast<std::int64_t>(model.snapshot->items.size()))
TX_MODEL_QUERY(list, begin_page, begin_page(model))
TX_MODEL_QUERY(table, begin_page, begin_page(model))
TX_MODEL_QUERY(tree, begin_page, begin_page(model))
#undef TX_MODEL_QUERY

#define TX_MODEL_CLOSE(kind) \
extern "C" int txrt_gui_data_close_##kind##_model(graphics::resource* value) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        auto& model = *static_cast<data_model*>(value); \
        graphics::require_thread(model.thread); \
        if (!model.closed) \
        { \
            close_model(model); \
        } \
    }); \
}

TX_MODEL_CLOSE(list)
TX_MODEL_CLOSE(table)
TX_MODEL_CLOSE(tree)
#undef TX_MODEL_CLOSE

#define TX_VIEW_SELECTION(kind) \
extern "C" int txrt_gui_data_selected_ids_##kind##_view(graphics::resource* value, void** result) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        int_vector ids; \
        ids.data().values = require_node(value).selection; \
        ids.data().refresh(); \
        *result = detail::make_handle<std::any>(std::move(ids)); \
    }); \
} \
extern "C" int txrt_gui_data_set_selected_ids_##kind##_view(graphics::resource* value, const void* ids) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        set_selected_ids(require_node(value), read_ids(ids)); \
    }); \
}

TX_VIEW_SELECTION(list)
TX_VIEW_SELECTION(table)
TX_VIEW_SELECTION(tree)
#undef TX_VIEW_SELECTION
