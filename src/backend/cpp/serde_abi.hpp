#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace tx_generated
{

enum class serde_kind : std::int64_t
{
    integer, floating, boolean, text, bytes, vector, option, structure
};

enum class serde_unknown : std::int64_t
{
    reject, ignore, preserve
};

struct serde_schema;
struct serde_codec;
struct record_type;

struct serde_type
{
    serde_kind kind;
    const char* name;
    const serde_type* element;
    const serde_schema* structure;
    // 编译期绑定编解码入口；字段仍按 index 访问现有结构体槽。
    const serde_codec* codec;
};

struct serde_default
{
    std::int64_t kind;
    std::int64_t bits;
    const char* text;
    std::uint64_t length;
};

struct serde_field
{
    const char* name;
    std::int64_t number;
    std::uint64_t index;
    serde_type type;
    serde_default default_value;
};

struct serde_schema
{
    const char* type_name;
    const char* display_name;
    std::int64_t version;
    serde_unknown unknown;
    std::uint64_t field_count;
    std::int64_t unknown_index;
    const char* unknown_name;
    const serde_field* field_data;
    std::uint64_t field_size;
    const std::uint64_t* json_order;
    const std::uint64_t* cbor_order;
    const record_type* layout = nullptr;

    [[nodiscard]] std::span<const serde_field> fields() const noexcept
    {
        return {field_data, field_size};
    }
};

// 与编译器生成的只读 LLVM 常量一致，不依赖 C++ 容器布局。
static_assert(std::is_standard_layout_v<serde_schema> && sizeof(serde_schema) == 96);
static_assert(offsetof(serde_schema, field_data) == 56);
static_assert(sizeof(serde_type) == 40 && sizeof(serde_default) == 32);
static_assert(sizeof(serde_field) == 96 && offsetof(serde_field, default_value) == 64);

} // namespace tx_generated

extern "C"
{

int txrt_serde_serialize_json(const tx_generated::serde_schema* schema, const void* value,
                              void** result) noexcept;
int txrt_serde_deserialize_json(const tx_generated::serde_schema* schema, const void* text,
                                void** result) noexcept;
int txrt_serde_serialize_cbor(const tx_generated::serde_schema* schema, const void* value,
                              void** result) noexcept;
int txrt_serde_deserialize_cbor(const tx_generated::serde_schema* schema, const void* bytes,
                                void** result) noexcept;

}
