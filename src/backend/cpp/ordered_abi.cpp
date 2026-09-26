#include "backend/cpp/container_abi_internal.hpp"
#include "stdlib/typed_ordered.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace
{

using namespace tx_generated;
using namespace tx_generated::detail;

const std::any& object_value(const void* value)
{
    if (!value)
    {
        throw std::runtime_error("有序容器元素不能为 none");
    }
    return *static_cast<const std::any*>(value);
}

template<class value_type>
void write_value(const value_type& value, void* result)
{
    if constexpr (std::is_same_v<value_type, std::any>)
    {
        *static_cast<void**>(result) = make_handle<std::any>(value);
    }
    else if constexpr (std::is_same_v<value_type, text_reference>)
    {
        *static_cast<void**>(result) = retain_text_handle(value.handle());
    }
    else if constexpr (std::is_same_v<value_type, std::uint8_t>)
    {
        *static_cast<bool*>(result) = value != 0;
    }
    else
    {
        *static_cast<value_type*>(result) = value;
    }
}

template<class storage_type, class... names>
int new_ordered(const void* less, const void* closure,
                void** result, names... type_names) noexcept
{
    return invoke_checked([&]
    {
        if ((!type_names || ...))
        {
            throw std::runtime_error("有序容器缺少静态类型名");
        }
        const auto callback = closure ? object_value(closure) : std::any{};
        auto storage = std::make_shared<storage_type>(
            type_names..., less, callback);
        register_ordered_storage(storage);
        container_handle container = std::move(storage);
        *result = make_handle<std::any>(std::move(container));
    });
}

} // namespace

#define TX_ORDERED_MAP(KEY_SUFFIX, VALUE_SUFFIX, KEY_TYPE, VALUE_TYPE, KEY_ABI, VALUE_ABI, KEY_EXPR, VALUE_EXPR, FALLBACK_EXPR) \
extern "C" int txrt_ordered_map_new_##KEY_SUFFIX##_##VALUE_SUFFIX( \
    const char* key_name, const char* value_name, const void* less, \
    const void* closure, void** result) noexcept \
{ \
    return new_ordered<ordered_map_storage<KEY_TYPE, VALUE_TYPE>>( \
        less, closure, result, key_name, value_name); \
} \
extern "C" int txrt_ordered_map_size_##KEY_SUFFIX##_##VALUE_SUFFIX( \
    const void* value, std::int64_t* result) noexcept \
{ \
    return container_apply<ordered_map_storage<KEY_TYPE, VALUE_TYPE>>(value, [&](auto& data) \
    { \
        data.require_ready(); \
        container_result(data.values.size(), result); \
    }); \
} \
extern "C" int txrt_ordered_map_empty_##KEY_SUFFIX##_##VALUE_SUFFIX( \
    const void* value, bool* result) noexcept \
{ \
    return container_apply<ordered_map_storage<KEY_TYPE, VALUE_TYPE>>(value, [&](auto& data) \
    { \
        data.require_ready(); \
        *result = data.values.empty(); \
    }); \
} \
extern "C" int txrt_ordered_map_clear_##KEY_SUFFIX##_##VALUE_SUFFIX(const void* value) noexcept \
{ \
    return container_apply<ordered_map_storage<KEY_TYPE, VALUE_TYPE>>(value, [&](auto& data) \
    { \
        data.require_ready(); \
        data.values.clear(); \
    }); \
} \
extern "C" int txrt_ordered_map_contains_##KEY_SUFFIX##_##VALUE_SUFFIX( \
    const void* value, KEY_ABI key, bool* result) noexcept \
{ \
    return container_apply<ordered_map_storage<KEY_TYPE, VALUE_TYPE>>(value, [&](auto& data) \
    { \
        data.require_ready(); \
        const auto item = KEY_EXPR; \
        require_ordered_key(item); \
        *result = data.values.contains(item); \
    }); \
} \
extern "C" int txrt_ordered_map_remove_##KEY_SUFFIX##_##VALUE_SUFFIX( \
    const void* value, KEY_ABI key, bool* result) noexcept \
{ \
    return container_apply<ordered_map_storage<KEY_TYPE, VALUE_TYPE>>(value, [&](auto& data) \
    { \
        data.require_ready(); \
        const auto item = KEY_EXPR; \
        require_ordered_key(item); \
        *result = data.values.erase(item) != 0; \
    }); \
} \
extern "C" int txrt_ordered_map_set_##KEY_SUFFIX##_##VALUE_SUFFIX( \
    const void* value, KEY_ABI key, VALUE_ABI item) noexcept \
{ \
    return container_apply<ordered_map_storage<KEY_TYPE, VALUE_TYPE>>(value, [&](auto& data) \
    { \
        data.set(KEY_EXPR, VALUE_EXPR); \
    }); \
} \
extern "C" int txrt_ordered_map_read_##KEY_SUFFIX##_##VALUE_SUFFIX( \
    const void* value, KEY_ABI key, void* result) noexcept \
{ \
    return container_apply<ordered_map_storage<KEY_TYPE, VALUE_TYPE>>(value, [&](auto& data) \
    { \
        write_value(data.read(KEY_EXPR), result); \
    }); \
} \
extern "C" int txrt_ordered_map_get_##KEY_SUFFIX##_##VALUE_SUFFIX( \
    const void* value, KEY_ABI key, VALUE_ABI fallback, void* result) noexcept \
{ \
    return container_apply<ordered_map_storage<KEY_TYPE, VALUE_TYPE>>(value, [&](auto& data) \
    { \
        write_value(data.get(KEY_EXPR, FALLBACK_EXPR), result); \
    }); \
} \
extern "C" int txrt_ordered_map_keys_##KEY_SUFFIX##_##VALUE_SUFFIX( \
    const void* value, void** result) noexcept \
{ \
    return container_apply<ordered_map_storage<KEY_TYPE, VALUE_TYPE>>(value, [&](auto& data) \
    { \
        container_result(data.keys(), result); \
    }); \
} \
extern "C" int txrt_ordered_map_values_##KEY_SUFFIX##_##VALUE_SUFFIX( \
    const void* value, void** result) noexcept \
{ \
    return container_apply<ordered_map_storage<KEY_TYPE, VALUE_TYPE>>(value, [&](auto& data) \
    { \
        container_result(data.values_snapshot(), result); \
    }); \
} \
extern "C" int txrt_ordered_map_entries_##KEY_SUFFIX##_##VALUE_SUFFIX( \
    const void* value, void** result) noexcept \
{ \
    return container_apply<ordered_map_storage<KEY_TYPE, VALUE_TYPE>>(value, [&](auto& data) \
    { \
        container_result(data.entries(), result); \
    }); \
} \
extern "C" int txrt_ordered_map_range_##KEY_SUFFIX##_##VALUE_SUFFIX( \
    const void* value, KEY_ABI low, KEY_ABI high, void** result) noexcept \
{ \
    return container_apply<ordered_map_storage<KEY_TYPE, VALUE_TYPE>>(value, [&](auto& data) \
    { \
        const auto first = ([&] { KEY_ABI key = low; return KEY_EXPR; })(); \
        const auto second = ([&] { KEY_ABI key = high; return KEY_EXPR; })(); \
        container_result(data.range(first, second), result); \
    }); \
}

TX_ORDERED_MAP(i64, i64, std::int64_t, std::int64_t, std::int64_t, std::int64_t,
    key, item, fallback)
TX_ORDERED_MAP(i64, f64, std::int64_t, double, std::int64_t, double,
    key, item, fallback)
TX_ORDERED_MAP(i64, bool, std::int64_t, std::uint8_t, std::int64_t, bool,
    key, static_cast<std::uint8_t>(item), static_cast<std::uint8_t>(fallback))
TX_ORDERED_MAP(i64, str, std::int64_t, text_reference, std::int64_t, const void*,
    key, text_reference(item), text_reference(fallback))
TX_ORDERED_MAP(i64, object, std::int64_t, std::any, std::int64_t, const void*,
    key, object_value(item), object_value(fallback))
TX_ORDERED_MAP(f64, i64, double, std::int64_t, double, std::int64_t,
    key, item, fallback)
TX_ORDERED_MAP(f64, f64, double, double, double, double,
    key, item, fallback)
TX_ORDERED_MAP(f64, bool, double, std::uint8_t, double, bool,
    key, static_cast<std::uint8_t>(item), static_cast<std::uint8_t>(fallback))
TX_ORDERED_MAP(f64, str, double, text_reference, double, const void*,
    key, text_reference(item), text_reference(fallback))
TX_ORDERED_MAP(f64, object, double, std::any, double, const void*,
    key, object_value(item), object_value(fallback))
TX_ORDERED_MAP(bool, i64, std::uint8_t, std::int64_t, bool, std::int64_t,
    static_cast<std::uint8_t>(key), item, fallback)
TX_ORDERED_MAP(bool, f64, std::uint8_t, double, bool, double,
    static_cast<std::uint8_t>(key), item, fallback)
TX_ORDERED_MAP(bool, bool, std::uint8_t, std::uint8_t, bool, bool,
    static_cast<std::uint8_t>(key), static_cast<std::uint8_t>(item), static_cast<std::uint8_t>(fallback))
TX_ORDERED_MAP(bool, str, std::uint8_t, text_reference, bool, const void*,
    static_cast<std::uint8_t>(key), text_reference(item), text_reference(fallback))
TX_ORDERED_MAP(bool, object, std::uint8_t, std::any, bool, const void*,
    static_cast<std::uint8_t>(key), object_value(item), object_value(fallback))
TX_ORDERED_MAP(str, i64, text_reference, std::int64_t, const void*, std::int64_t,
    text_reference(key), item, fallback)
TX_ORDERED_MAP(str, f64, text_reference, double, const void*, double,
    text_reference(key), item, fallback)
TX_ORDERED_MAP(str, bool, text_reference, std::uint8_t, const void*, bool,
    text_reference(key), static_cast<std::uint8_t>(item), static_cast<std::uint8_t>(fallback))
TX_ORDERED_MAP(str, str, text_reference, text_reference, const void*, const void*,
    text_reference(key), text_reference(item), text_reference(fallback))
TX_ORDERED_MAP(str, object, text_reference, std::any, const void*, const void*,
    text_reference(key), object_value(item), object_value(fallback))
TX_ORDERED_MAP(object, i64, std::any, std::int64_t, const void*, std::int64_t,
    object_value(key), item, fallback)
TX_ORDERED_MAP(object, f64, std::any, double, const void*, double,
    object_value(key), item, fallback)
TX_ORDERED_MAP(object, bool, std::any, std::uint8_t, const void*, bool,
    object_value(key), static_cast<std::uint8_t>(item), static_cast<std::uint8_t>(fallback))
TX_ORDERED_MAP(object, str, std::any, text_reference, const void*, const void*,
    object_value(key), text_reference(item), text_reference(fallback))
TX_ORDERED_MAP(object, object, std::any, std::any, const void*, const void*,
    object_value(key), object_value(item), object_value(fallback))

#undef TX_ORDERED_MAP

#define TX_ORDERED_SET(SUFFIX, KEY_TYPE, KEY_ABI, KEY_EXPR) \
extern "C" int txrt_ordered_set_new_##SUFFIX(const char* key_name, \
    const void* less, const void* closure, void** result) noexcept \
{ \
    return new_ordered<ordered_set_storage<KEY_TYPE>>( \
        less, closure, result, key_name); \
} \
extern "C" int txrt_ordered_set_size_##SUFFIX(const void* value, \
    std::int64_t* result) noexcept \
{ \
    return container_apply<ordered_set_storage<KEY_TYPE>>(value, [&](auto& data) \
    { \
        data.require_ready(); \
        container_result(data.values.size(), result); \
    }); \
} \
extern "C" int txrt_ordered_set_empty_##SUFFIX(const void* value, bool* result) noexcept \
{ \
    return container_apply<ordered_set_storage<KEY_TYPE>>(value, [&](auto& data) \
    { \
        data.require_ready(); \
        *result = data.values.empty(); \
    }); \
} \
extern "C" int txrt_ordered_set_clear_##SUFFIX(const void* value) noexcept \
{ \
    return container_apply<ordered_set_storage<KEY_TYPE>>(value, [&](auto& data) \
    { \
        data.require_ready(); \
        data.values.clear(); \
    }); \
} \
extern "C" int txrt_ordered_set_contains_##SUFFIX(const void* value, \
    KEY_ABI key, bool* result) noexcept \
{ \
    return container_apply<ordered_set_storage<KEY_TYPE>>(value, [&](auto& data) \
    { \
        data.require_ready(); \
        const auto item = KEY_EXPR; \
        require_ordered_key(item); \
        *result = data.values.contains(item); \
    }); \
} \
extern "C" int txrt_ordered_set_remove_##SUFFIX(const void* value, \
    KEY_ABI key, bool* result) noexcept \
{ \
    return container_apply<ordered_set_storage<KEY_TYPE>>(value, [&](auto& data) \
    { \
        data.require_ready(); \
        const auto item = KEY_EXPR; \
        require_ordered_key(item); \
        *result = data.values.erase(item) != 0; \
    }); \
} \
extern "C" int txrt_ordered_set_insert_##SUFFIX(const void* value, \
    KEY_ABI key, bool* result) noexcept \
{ \
    return container_apply<ordered_set_storage<KEY_TYPE>>(value, [&](auto& data) \
    { \
        *result = data.insert(KEY_EXPR); \
    }); \
} \
extern "C" int txrt_ordered_set_to_vector_##SUFFIX(const void* value, \
    void** result) noexcept \
{ \
    return container_apply<ordered_set_storage<KEY_TYPE>>(value, [&](auto& data) \
    { \
        container_result(data.snapshot(), result); \
    }); \
} \
extern "C" int txrt_ordered_set_range_##SUFFIX(const void* value, \
    KEY_ABI low, KEY_ABI high, void** result) noexcept \
{ \
    return container_apply<ordered_set_storage<KEY_TYPE>>(value, [&](auto& data) \
    { \
        const auto first = ([&] { KEY_ABI key = low; return KEY_EXPR; })(); \
        const auto second = ([&] { KEY_ABI key = high; return KEY_EXPR; })(); \
        container_result(data.range(first, second), result); \
    }); \
}

TX_ORDERED_SET(i64, std::int64_t, std::int64_t, key)
TX_ORDERED_SET(f64, double, double, key)
TX_ORDERED_SET(bool, std::uint8_t, bool, static_cast<std::uint8_t>(key))
TX_ORDERED_SET(str, text_reference, const void*, text_reference(key))
TX_ORDERED_SET(object, std::any, const void*, object_value(key))

#undef TX_ORDERED_SET
