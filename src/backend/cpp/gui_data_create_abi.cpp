#include "backend/cpp/gui_data_bridge.hpp"

using namespace tx_generated;
using namespace tx_generated::gui;

extern "C" int txrt_gui_data_create_list_model(graphics::resource* window, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(create_model(graphics::require_window(window), tx::graphics_kind::list_model));
    });
}

extern "C" int txrt_gui_data_create_tree_model(graphics::resource* window, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(create_model(graphics::require_window(window), tx::graphics_kind::tree_model));
    });
}

extern "C" int txrt_gui_data_create_table_model(graphics::resource* window, const void* value, const char* type, void** result) noexcept
{
    return graphics::result_call(type, result, [&]() -> std::any
    {
        const auto& records = detail::vector_value<std::any>(value).data().values;
        if (records.empty() || records.size() > 256)
        {
            fail("invalid_argument", "表格需要 1–256 列");
        }
        std::vector<data_column> columns;
        for (const auto& value : records)
        {
            const auto& record = std::any_cast<const dynamic_struct&>(value);
            data_column column;
            column.id = std::any_cast<std::int64_t>(record->read_field(0));
            column.title = wide_text(std::any_cast<std::string>(record->read_field(1)));
            column.width = std::any_cast<double>(record->read_field(2));
            const auto alignment = std::any_cast<std::string>(record->read_field(3));
            if (alignment != "left" && alignment != "center" && alignment != "right")
            {
                fail("invalid_argument", "列对齐必须为 left/center/right");
            }
            column.alignment = alignment == "left" ? LVCFMT_LEFT : alignment == "center" ? LVCFMT_CENTER : LVCFMT_RIGHT;
            columns.push_back(std::move(column));
        }
        return graphics::make_handle(create_model(graphics::require_window(window),
            tx::graphics_kind::table_model, std::move(columns)));
    });
}

// 各视图在编译期选定类型，Windows 取数只读取已提交的模型快照。
#define TX_CREATE_VIEW(kind) \
extern "C" int txrt_gui_data_create_##kind##_view(graphics::resource* parent, graphics::resource* model, bool multiple, const char* type, void** result) noexcept \
{ \
    return graphics::result_call(type, result, [&]() -> std::any \
    { \
        return graphics::make_handle(create_view(require_node(parent), require_model(model), tx::graphics_kind::kind##_view, multiple)); \
    }); \
}

TX_CREATE_VIEW(list)
TX_CREATE_VIEW(table)
TX_CREATE_VIEW(tree)
#undef TX_CREATE_VIEW
