#pragma once

#include "stdlib/graphics/resource.hpp"

#include <cstdint>

extern "C"
{
void* txrt_graphics_resource_view(const void* value) noexcept;
int txrt_graphics_open_app(const char* result_type, void** result) noexcept;
int txrt_graphics_close_app(tx_generated::graphics::resource* value) noexcept;
int txrt_graphics_close_window(tx_generated::graphics::resource* value) noexcept;
int txrt_graphics_create_window(tx_generated::graphics::resource* value,
    const void* title, double width, double height, bool resizable,
    const char* result_type, void** result) noexcept;
int txrt_graphics_is_open(tx_generated::graphics::resource* value, bool* result) noexcept;
int txrt_graphics_show(tx_generated::graphics::resource* value,
    const char* result_type, void** result) noexcept;
int txrt_graphics_set_title(tx_generated::graphics::resource* value, const void* title,
    const char* result_type, void** result) noexcept;
int txrt_graphics_window_id(tx_generated::graphics::resource* value, std::int64_t* result) noexcept;
int txrt_graphics_invalidate(tx_generated::graphics::resource* value) noexcept;
int txrt_graphics_backend(tx_generated::graphics::resource* value, void** result) noexcept;
int txrt_graphics_last_native_error(std::int64_t* result) noexcept;
int txrt_graphics_begin_frame(tx_generated::graphics::resource* value,
    const char* result_type, void** result) noexcept;
int txrt_graphics_end_frame(tx_generated::graphics::resource* value,
    const char* result_type, void** result) noexcept;
int txrt_graphics_cancel_frame(tx_generated::graphics::resource* value) noexcept;
int txrt_graphics_next_event(tx_generated::graphics::resource* value, std::int64_t timeout,
    const char* result_type, const char* event_type, const char* resize_type,
    const char* pointer_type, const char* key_type, const char* text_type,
    const char* control_type, void** result) noexcept;
}
