#include "backend/cpp/gui_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/gui/windows/state.hpp"

extern "C" int txrt_gui_set_theme(tx_generated::graphics::resource* root,
    const void* theme) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        tx_generated::gui::set_theme(tx_generated::gui::require_node(root),
            tx_generated::detail::text_value(theme));
    });
}

extern "C" int txrt_gui_set_button_appearance(tx_generated::graphics::resource* control,
    const void* appearance) noexcept
{
    return tx_generated::detail::invoke_leaf([&]
    {
        tx_generated::gui::set_button_appearance(tx_generated::gui::require_node(control),
            tx_generated::detail::text_value(appearance));
    });
}
