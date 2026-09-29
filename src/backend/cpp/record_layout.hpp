#pragma once

#include "backend/cpp/typed_slots.hpp"
#include "backend/cpp/cycle_gc.hpp"
#include "common/record_copy_kind.hpp"

#include <string_view>

namespace tx_generated
{

struct record_field
{
    const char* name;
    const char* type_name;
    std::uint64_t offset;
    std::uint64_t kind;
    tx::record_copy_kind copy_kind;
};

struct record_type
{
    const char* name;
    const char* display_name;
    const record_field* fields;
    std::uint64_t field_count;
    const record_type* const* ancestors;
    const char* const* ancestor_names;
    std::uint64_t ancestor_count;
    const void* const* virtual_targets;
    std::uint64_t virtual_count;
    const void* const* destructors;
    std::uint64_t destructor_count;
    const std::uint64_t* scan_indices;
    std::uint64_t scan_count;
    const slot_kind* kinds;
};

static_assert(sizeof(record_field) == 40 && offsetof(record_field, offset) == 16);
static_assert(sizeof(record_type) == 112 && offsetof(record_type, virtual_targets) == 56);

struct record_storage
{
    const record_type* type = nullptr;
    typed_slots slots;

    record_storage() = default;
    explicit record_storage(const record_type* description);
    void scan(gc_visit visit, void* context) const;
};

// 公开 ABI 返回稳定存储头，生成代码只读取 data / type；对象身份和所有权仍由句柄维护。
struct record_view
{
    typed_slot* data = nullptr;
    const record_type* type = nullptr;
};

static_assert(sizeof(record_view) == 16);

} // namespace tx_generated
