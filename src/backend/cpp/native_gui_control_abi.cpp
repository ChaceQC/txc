#include "backend/cpp/native_gui_abi.hpp"
#include "backend/cpp/native_gui_result.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/native_gui/state.hpp"

using namespace tx_generated;
using graphics::resource;

namespace
{
int set_size(resource* control, double width, double height) noexcept
{
    return native_gui::leaf_call([&]
    {
        auto& state = native_gui::require_node(control);
        native_gui::require_idle(state);
        native_gui::checked_size(width);
        native_gui::checked_size(height);
        state.width = width > 0 ? gui::length{gui::length_mode::fixed, width} : gui::length{};
        state.height = height > 0 ? gui::length{gui::length_mode::fixed, height} : gui::length{};
        native_gui::dirty(state);
    });
}
}

// 有限类型在编译期展开为独立 ABI 符号，运行时不做类型或方法名查找。
#define tx_native_gui_common(kind) \
extern "C" int txrt_native_gui_id_##kind(resource* control, std::int64_t* result) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        *result = native_gui::require_node(control).id; \
    }); \
} \
extern "C" int txrt_native_gui_close_##kind(resource* control) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        native_gui::close(native_gui::require_node(control, true)); \
    }); \
} \
extern "C" int txrt_native_gui_set_size_##kind(resource* control, double width, double height) noexcept \
{ \
    return set_size(control, width, height); \
} \
extern "C" int txrt_native_gui_set_visible_##kind(resource* control, bool visible) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        native_gui::set_visible(native_gui::require_node(control), visible); \
    }); \
} \
extern "C" int txrt_native_gui_set_enabled_##kind(resource* control, bool enabled) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        native_gui::set_enabled(native_gui::require_node(control), enabled); \
    }); \
}

tx_native_gui_common(panel)
tx_native_gui_common(label)
tx_native_gui_common(button)
tx_native_gui_common(check_box)
tx_native_gui_common(text_box)
tx_native_gui_common(progress_bar)
tx_native_gui_common(slider)
tx_native_gui_common(radio_button)
#undef tx_native_gui_common

#define tx_native_gui_text(kind) \
extern "C" int txrt_native_gui_set_text_##kind(resource* control, const void* text) noexcept \
{ \
    return native_gui::leaf_call([&] \
    { \
        native_gui::set_text(native_gui::require_node(control), detail::text_value(text)); \
    }); \
}

tx_native_gui_text(label)
tx_native_gui_text(button)
tx_native_gui_text(check_box)
tx_native_gui_text(text_box)
tx_native_gui_text(radio_button)
#undef tx_native_gui_text
