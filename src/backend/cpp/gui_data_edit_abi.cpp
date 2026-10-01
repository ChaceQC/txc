#include "backend/cpp/gui_data_bridge.hpp"

using namespace tx_generated;
using namespace tx_generated::gui;

#define TX_MODEL_EDIT(kind, operation, edit) \
extern "C" int txrt_gui_data_##operation##_##kind##_model(graphics::resource* value, const void* items) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        auto& model = require_model(value); \
        edit_model(model, read_items<tx::graphics_kind::kind##_model>(items), model_edit::edit); \
    }); \
}

TX_MODEL_EDIT(list, replace_items, replace)
TX_MODEL_EDIT(list, append_items, append)
TX_MODEL_EDIT(list, update_items, update)
TX_MODEL_EDIT(table, replace_items, replace)
TX_MODEL_EDIT(table, append_items, append)
TX_MODEL_EDIT(table, update_items, update)
TX_MODEL_EDIT(tree, replace_items, replace)
TX_MODEL_EDIT(tree, append_items, append)
TX_MODEL_EDIT(tree, update_items, update)
#undef TX_MODEL_EDIT

#define TX_MODEL_IDS(kind, operation) \
extern "C" int txrt_gui_data_##operation##_##kind##_model(graphics::resource* value, const void* ids) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        operation(require_model(value), read_ids(ids)); \
    }); \
}

TX_MODEL_IDS(list, remove_items)
TX_MODEL_IDS(list, set_order)
TX_MODEL_IDS(table, remove_items)
TX_MODEL_IDS(table, set_order)
TX_MODEL_IDS(tree, remove_items)
TX_MODEL_IDS(tree, set_order)
#undef TX_MODEL_IDS

#define TX_APPLY_PAGE(kind) \
extern "C" int txrt_gui_data_apply_page_##kind##_model(graphics::resource* value, std::int64_t request, std::int64_t revision, const void* items) noexcept \
{ \
    return detail::invoke_leaf([&] \
    { \
        auto& model = require_model(value); \
        if (!request || request != model.request_id || revision != model.revision) \
        { \
            fail("stale_revision", "分页请求已过期"); \
        } \
        apply_page(model, request, revision, read_items<tx::graphics_kind::kind##_model>(items)); \
    }); \
}

TX_APPLY_PAGE(list)
TX_APPLY_PAGE(table)
TX_APPLY_PAGE(tree)
#undef TX_APPLY_PAGE
