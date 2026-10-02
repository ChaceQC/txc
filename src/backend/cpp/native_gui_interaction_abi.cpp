#include "backend/cpp/native_gui_result.hpp"
#include "backend/cpp/native_gui_interaction_abi.hpp"
#include "backend/cpp/vector_abi_internal.hpp"
#include "stdlib/native_gui/combo.hpp"

using namespace tx_generated;
using graphics::resource;

extern "C" int txrt_native_gui_create_combo_box(resource* parent, const char* type, void** result) noexcept
{
    return native_gui::result_call(type, result, [&]() -> std::any
    {
        return graphics::make_handle(native_gui::create(native_gui::require_node(parent),
            tx::graphics_kind::native_combo_box, ""));
    });
}

extern "C" int txrt_native_gui_set_combo_items(resource* control, const void* items) noexcept
{
    return native_gui::leaf_call([&]
    {
        const auto& source = detail::vector_value<text_reference>(items).data().values;
        if (source.size() > 10000)
        {
            native_gui::fail("resource_limit", "下拉框最多支持 10000 项");
        }
        std::vector<std::u32string> converted;
        std::size_t bytes = 0;
        for (const auto& item : source)
        {
            bytes += item.get().size();
            if (item.get().size() > 65536 || bytes > 16 * 1024 * 1024)
            {
                native_gui::fail("resource_limit", "下拉选项文字超过大小上限");
            }
            converted.push_back(tx::ui::decode_utf8(item.get()).scalars);
        }
        native_gui::set_combo_items(native_gui::require_node(control), std::move(converted));
    });
}

extern "C" int txrt_native_gui_selected_index(resource* control, std::int64_t* result) noexcept
{
    return native_gui::leaf_call([&]
    {
        *result = native_gui::require_node(control).combo->selected;
    });
}

extern "C" int txrt_native_gui_set_selected_index(resource* control, std::int64_t index) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        native_gui::select_combo(state, index, false);
        native_gui::close_combo(native_gui::root_node(state));
    });
}

extern "C" int txrt_native_gui_set_default_button(resource* control, bool enabled) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_action_button(native_gui::require_node(control), false, enabled);
    });
}

extern "C" int txrt_native_gui_set_cancel_button(resource* control, bool enabled) noexcept
{
    return native_gui::leaf_call([&]
    {
        native_gui::set_action_button(native_gui::require_node(control), true, enabled);
    });
}

// 控件类型由编译器选定，公共实现只处理已经解析好的访问键。
#define tx_access_key(kind) \
extern "C" int txrt_native_gui_set_access_key_##kind(resource* control, const void* key) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        native_gui::set_access_key(native_gui::require_node(control), detail::text_value(key)); \
    }); \
}
tx_access_key(button)
tx_access_key(check_box)
tx_access_key(radio_button)
tx_access_key(text_box)
tx_access_key(slider)
tx_access_key(combo_box)
#undef tx_access_key
