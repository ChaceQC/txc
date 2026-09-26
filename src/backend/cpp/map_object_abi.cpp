#include "backend/cpp/container_abi_internal.hpp"
#include "stdlib/typed_map.hpp"
#include "stdlib/object_key_map.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace
{

using namespace tx_generated;
using namespace tx_generated::detail;

using i64_map = map_storage<std::int64_t, std::any>;
using f64_map = map_storage<double, std::any>;
using bool_map = map_storage<std::uint8_t, std::any>;
using str_map = map_storage<text_reference, std::any>;
using object_map = object_key_map<std::any>;

const std::any& object_value(const void* value)
{
    if (!value)
    {
        throw std::runtime_error("map 值不能为 none");
    }
    return *static_cast<const std::any*>(value);
}

std::int64_t key_value(std::int64_t value)
{
    return value;
}

double key_value(double value)
{
    return value;
}

std::uint8_t key_value(bool value)
{
    return value;
}

text_reference text_key(const void* value)
{
    return text_reference(value);
}

const std::any& object_key(const void* value)
{
    if (!value)
    {
        throw std::runtime_error("map 键不能为 none");
    }
    return *static_cast<const std::any*>(value);
}

template<class storage_type>
int new_scalar_map(const char* value_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        if (!value_name)
        {
            throw std::runtime_error("map 缺少值类型");
        }
        auto storage = std::make_shared<storage_type>(value_name);
        register_gc_node(storage,
            [](const void* object, gc_visit visit, void* context)
            {
                for (const auto& [key, value] :
                     static_cast<const storage_type*>(object)->values)
                {
                    (void)key;
                    visit(value, context);
                }
            },
            [](void* object)
            {
                static_cast<storage_type*>(object)->values.clear();
            });
        container_handle container = std::move(storage);
        *result = make_handle<std::any>(std::move(container));
    });
}

template<class storage_type>
auto map_keys(storage_type& data)
{
    if constexpr (requires { data.template snapshot<true>(); })
    {
        return data.template snapshot<true>();
    }
    else
    {
        return data.keys();
    }
}

template<class storage_type>
auto map_values(storage_type& data)
{
    if constexpr (requires { data.template snapshot<false>(); })
    {
        return data.template snapshot<false>();
    }
    else
    {
        return data.values_snapshot();
    }
}

} // namespace

#define TX_MAP_OBJECT(SUFFIX, STORAGE, ABI, KEY) \
extern "C" int txrt_map_size_##SUFFIX(const void* value, std::int64_t* result) noexcept \
{ \
    return container_apply<STORAGE>(value, [&](auto& data) \
    { \
        container_result(data.values.size(), result); \
    }); \
} \
extern "C" int txrt_map_empty_##SUFFIX(const void* value, bool* result) noexcept \
{ \
    return container_apply<STORAGE>(value, [&](auto& data) \
    { \
        *result = data.values.empty(); \
    }); \
} \
extern "C" int txrt_map_clear_##SUFFIX(const void* value) noexcept \
{ \
    return container_apply<STORAGE>(value, [&](auto& data) \
    { \
        data.values.clear(); \
    }); \
} \
extern "C" int txrt_map_contains_##SUFFIX(const void* value, ABI key, bool* result) noexcept \
{ \
    return container_apply<STORAGE>(value, [&](auto& data) \
    { \
        const auto item = KEY; \
        require_ordered_key(item); \
        *result = data.values.contains(item); \
    }); \
} \
extern "C" int txrt_map_remove_##SUFFIX(const void* value, ABI key, bool* result) noexcept \
{ \
    return container_apply<STORAGE>(value, [&](auto& data) \
    { \
        const auto item = KEY; \
        require_ordered_key(item); \
        *result = data.values.erase(item) != 0; \
    }); \
} \
extern "C" int txrt_map_set_##SUFFIX(const void* value, ABI key, const void* item) noexcept \
{ \
    return container_apply<STORAGE>(value, [&](auto& data) \
    { \
        data.set(KEY, object_value(item)); \
    }); \
} \
extern "C" int txrt_map_read_##SUFFIX(const void* value, ABI key, void** result) noexcept \
{ \
    return container_apply<STORAGE>(value, [&](auto& data) \
    { \
        *result = make_handle<std::any>(data.read(KEY)); \
    }); \
} \
extern "C" int txrt_map_get_##SUFFIX(const void* value, ABI key, \
    const void* fallback, void** result) noexcept \
{ \
    return container_apply<STORAGE>(value, [&](auto& data) \
    { \
        *result = make_handle<std::any>(data.get(KEY, object_value(fallback))); \
    }); \
} \
extern "C" int txrt_map_keys_##SUFFIX(const void* value, void** result) noexcept \
{ \
    return container_apply<STORAGE>(value, [&](auto& data) \
    { \
        container_result(map_keys(data), result); \
    }); \
} \
extern "C" int txrt_map_values_##SUFFIX(const void* value, void** result) noexcept \
{ \
    return container_apply<STORAGE>(value, [&](auto& data) \
    { \
        container_result(map_values(data), result); \
    }); \
} \
extern "C" int txrt_map_entries_##SUFFIX(const void* value, void** result) noexcept \
{ \
    return container_apply<STORAGE>(value, [&](auto& data) \
    { \
        container_result(data.entries(), result); \
    }); \
}

extern "C" int txrt_map_new_i64_object(const char* name, void** result) noexcept
{
    return new_scalar_map<i64_map>(name, result);
}

extern "C" int txrt_map_new_f64_object(const char* name, void** result) noexcept
{
    return new_scalar_map<f64_map>(name, result);
}

extern "C" int txrt_map_new_bool_object(const char* name, void** result) noexcept
{
    return new_scalar_map<bool_map>(name, result);
}

extern "C" int txrt_map_new_str_object(const char* name, void** result) noexcept
{
    return new_scalar_map<str_map>(name, result);
}

TX_MAP_OBJECT(i64_object, i64_map, std::int64_t, key_value(key))
TX_MAP_OBJECT(f64_object, f64_map, double, key_value(key))
TX_MAP_OBJECT(bool_object, bool_map, bool, key_value(key))
TX_MAP_OBJECT(str_object, str_map, const void*, text_key(key))
TX_MAP_OBJECT(object_object, object_map, const void*, object_key(key))

#undef TX_MAP_OBJECT
