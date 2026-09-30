#pragma once

#include "backend/cpp/record_layout.hpp"

extern "C"
{
int txrt_class_local_new(const tx_generated::record_type* type, void* storage, void** view) noexcept;
void txrt_class_local_destroy(void* storage,
    void (*destroy)(void*, tx_generated::record_view*)) noexcept;
}
