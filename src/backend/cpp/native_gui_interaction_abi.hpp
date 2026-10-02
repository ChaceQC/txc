#pragma once

#include "stdlib/graphics/resource.hpp"

extern "C"
{
int txrt_native_gui_create_combo_box(tx_generated::graphics::resource* parent, const char* type, void** result) noexcept;
int txrt_native_gui_set_combo_items(tx_generated::graphics::resource* control, const void* items) noexcept;
int txrt_native_gui_selected_index(tx_generated::graphics::resource* control, std::int64_t* result) noexcept;
int txrt_native_gui_set_selected_index(tx_generated::graphics::resource* control, std::int64_t index) noexcept;
int txrt_native_gui_set_default_button(tx_generated::graphics::resource* control, bool enabled) noexcept;
int txrt_native_gui_set_cancel_button(tx_generated::graphics::resource* control, bool enabled) noexcept;

#define tx_access_key_declare(kind) \
int txrt_native_gui_set_access_key_##kind(tx_generated::graphics::resource* control, const void* key) noexcept;
tx_access_key_declare(button)
tx_access_key_declare(check_box)
tx_access_key_declare(radio_button)
tx_access_key_declare(text_box)
tx_access_key_declare(slider)
tx_access_key_declare(combo_box)
#undef tx_access_key_declare
}
