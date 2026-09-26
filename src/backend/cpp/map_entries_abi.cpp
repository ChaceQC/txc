#include "backend/cpp/container_abi_internal.hpp"
#include "stdlib/typed_map.hpp"
#include "stdlib/object_key_map.hpp"

using namespace tx_generated;
using namespace tx_generated::detail;

#define TX_MAP_ENTRIES(SUFFIX, KEY, VALUE) \
extern "C" int txrt_map_entries_##SUFFIX(const void* value, void** result) noexcept \
{ \
    return container_apply<map_storage<KEY, VALUE>>(value, [&](auto& data) \
    { \
        container_result(data.entries(), result); \
    }); \
}

TX_MAP_ENTRIES(i64_i64, std::int64_t, std::int64_t)
TX_MAP_ENTRIES(i64_f64, std::int64_t, double)
TX_MAP_ENTRIES(i64_bool, std::int64_t, std::uint8_t)
TX_MAP_ENTRIES(i64_str, std::int64_t, text_reference)
TX_MAP_ENTRIES(f64_i64, double, std::int64_t)
TX_MAP_ENTRIES(f64_f64, double, double)
TX_MAP_ENTRIES(f64_bool, double, std::uint8_t)
TX_MAP_ENTRIES(f64_str, double, text_reference)
TX_MAP_ENTRIES(bool_i64, std::uint8_t, std::int64_t)
TX_MAP_ENTRIES(bool_f64, std::uint8_t, double)
TX_MAP_ENTRIES(bool_bool, std::uint8_t, std::uint8_t)
TX_MAP_ENTRIES(bool_str, std::uint8_t, text_reference)
TX_MAP_ENTRIES(str_i64, text_reference, std::int64_t)
TX_MAP_ENTRIES(str_f64, text_reference, double)
TX_MAP_ENTRIES(str_bool, text_reference, std::uint8_t)
TX_MAP_ENTRIES(str_str, text_reference, text_reference)
// 结构体键沿用其现有哈希/相等回调，条目键在快照中独立复制。
#define TX_OBJECT_MAP_ENTRIES(SUFFIX, VALUE) \
extern "C" int txrt_map_entries_object_##SUFFIX(const void* value, void** result) noexcept \
{ \
    return container_apply<object_key_map<VALUE>>(value, [&](auto& data) \
    { \
        container_result(data.entries(), result); \
    }); \
}

TX_OBJECT_MAP_ENTRIES(i64, std::int64_t)
TX_OBJECT_MAP_ENTRIES(f64, double)
TX_OBJECT_MAP_ENTRIES(bool, std::uint8_t)
TX_OBJECT_MAP_ENTRIES(str, text_reference)

#undef TX_OBJECT_MAP_ENTRIES

#undef TX_MAP_ENTRIES
