#include "backend/cpp/container_abi_internal.hpp"
#include "backend/cpp/value_abi.hpp"
#include "stdlib/object_key_map.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{

using namespace tx_generated;
using namespace tx_generated::detail;

using hash_callback = int (*)(const void*, std::int64_t*);
using equal_callback = int (*)(const void*, const void*, bool*);

void require_callback_success(int status)
{
    if (status != 0)
    {
        throw runtime_failure({last_error_kind, last_error_code, last_error});
    }
}

std::any copy_key(const std::any& key)
{
    void* copy = nullptr;
    require_callback_success(txrt_value_deep_copy(&key, &copy));
    const std::unique_ptr<std::any, decltype(&txrt_value_release)> owned(
        static_cast<std::any*>(copy), txrt_value_release);
    return *owned;
}

key_hash make_hash(const void* callback)
{
    if (!callback)
    {
        throw std::runtime_error("哈希键缺少 hash_key 回调");
    }
    const auto invoke = reinterpret_cast<hash_callback>(const_cast<void*>(callback));
    return [invoke](const std::any& key)
    {
        const auto stable = copy_key(key);
        std::int64_t hash = 0;
        require_callback_success(invoke(&stable, &hash));
        return static_cast<std::size_t>(hash);
    };
}

key_equal make_equal(const void* callback)
{
    if (!callback)
    {
        throw std::runtime_error("哈希键缺少 operator == 回调");
    }
    const auto invoke = reinterpret_cast<equal_callback>(const_cast<void*>(callback));
    return [invoke](const std::any& left, const std::any& right)
    {
        const auto stable_left = copy_key(left);
        const auto stable_right = copy_key(right);
        bool equal = false;
        require_callback_success(invoke(&stable_left, &stable_right, &equal));
        return equal;
    };
}

std::int64_t stored_value(std::int64_t value)
{
    return value;
}

double stored_value(double value)
{
    return value;
}

std::uint8_t stored_value(bool value)
{
    return value;
}

text_reference stored_value(const void* value)
{
    return text_reference(value);
}

template<class storage_type>
int make_container(const char* key_type, const void* hash,
                   const void* equal, void** result) noexcept
{
    return invoke_checked([&]
    {
        if (!key_type)
        {
            throw std::runtime_error("哈希键缺少静态类型名");
        }
        container_handle container = std::make_shared<storage_type>(
            std::string(key_type), make_hash(hash), make_equal(equal),
            key_copy(copy_key));
        *result = make_handle<std::any>(std::move(container));
        note_gc_allocation();
    });
}

} // namespace

extern "C" int txrt_map_new_object_object(const char* key_type,
    const void* hash, const void* equal, const char* value_type,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        if (!key_type || !value_type)
        {
            throw std::runtime_error("map 缺少键或值类型");
        }
        auto storage = std::make_shared<object_key_map<std::any>>(
            key_type, make_hash(hash), make_equal(equal),
            key_copy(copy_key), value_type);
        register_gc_node(storage,
            [](const void* object, gc_visit visit, void* context)
            {
                for (const auto& [key, value] :
                     static_cast<const object_key_map<std::any>*>(object)->values)
                {
                    (void)key;
                    visit(value, context);
                }
            },
            [](void* object)
            {
                static_cast<object_key_map<std::any>*>(object)->values.clear();
            });
        container_handle container = std::move(storage);
        *result = make_handle<std::any>(std::move(container));
    });
}

#define TX_OBJECT_MAP(SUFFIX, STORAGE, ABI, OUTPUT) \
extern "C" int txrt_map_new_object_##SUFFIX(const char* type, const void* hash, \
    const void* equal, void** result) noexcept \
{ \
    return make_container<object_key_map<STORAGE>>(type, hash, equal, result); \
} \
extern "C" int txrt_map_size_object_##SUFFIX(const void* value, \
    std::int64_t* result) noexcept \
{ \
    return container_apply<object_key_map<STORAGE>>(value, [&](auto& data) \
    { \
        container_result(data.values.size(), result); \
    }); \
} \
extern "C" int txrt_map_empty_object_##SUFFIX(const void* value, \
    bool* result) noexcept \
{ \
    return container_apply<object_key_map<STORAGE>>(value, [&](auto& data) \
    { \
        *result = data.values.empty(); \
    }); \
} \
extern "C" int txrt_map_clear_object_##SUFFIX(const void* value) noexcept \
{ \
    return container_apply<object_key_map<STORAGE>>(value, [&](auto& data) \
    { \
        data.values.clear(); \
    }); \
} \
extern "C" int txrt_map_contains_object_##SUFFIX(const void* value, \
    const void* key, bool* result) noexcept \
{ \
    return container_apply<object_key_map<STORAGE>>(value, [&](auto& data) \
    { \
        *result = data.values.contains(*static_cast<const std::any*>(key)); \
    }); \
} \
extern "C" int txrt_map_remove_object_##SUFFIX(const void* value, \
    const void* key, bool* result) noexcept \
{ \
    return container_apply<object_key_map<STORAGE>>(value, [&](auto& data) \
    { \
        *result = data.values.erase(*static_cast<const std::any*>(key)) != 0; \
    }); \
} \
extern "C" int txrt_map_set_object_##SUFFIX(const void* value, \
    const void* key, ABI item) noexcept \
{ \
    return container_apply<object_key_map<STORAGE>>(value, [&](auto& data) \
    { \
        data.set(*static_cast<const std::any*>(key), stored_value(item)); \
    }); \
} \
extern "C" int txrt_map_read_object_##SUFFIX(const void* value, \
    const void* key, OUTPUT result) noexcept \
{ \
    return container_apply<object_key_map<STORAGE>>(value, [&](auto& data) \
    { \
        container_result(data.read(*static_cast<const std::any*>(key)), result); \
    }); \
} \
extern "C" int txrt_map_get_object_##SUFFIX(const void* value, \
    const void* key, ABI fallback, OUTPUT result) noexcept \
{ \
    return container_apply<object_key_map<STORAGE>>(value, [&](auto& data) \
    { \
        container_result(data.get(*static_cast<const std::any*>(key), \
            stored_value(fallback)), result); \
    }); \
} \
extern "C" int txrt_map_keys_object_##SUFFIX(const void* value, \
    void** result) noexcept \
{ \
    return container_apply<object_key_map<STORAGE>>(value, [&](auto& data) \
    { \
        container_result(data.keys(), result); \
    }); \
} \
extern "C" int txrt_map_values_object_##SUFFIX(const void* value, \
    void** result) noexcept \
{ \
    return container_apply<object_key_map<STORAGE>>(value, [&](auto& data) \
    { \
        container_result(data.values_snapshot(), result); \
    }); \
}

TX_OBJECT_MAP(i64, std::int64_t, std::int64_t, std::int64_t*)
TX_OBJECT_MAP(f64, double, double, double*)
TX_OBJECT_MAP(bool, std::uint8_t, bool, bool*)
TX_OBJECT_MAP(str, text_reference, const void*, void**)

#undef TX_OBJECT_MAP

extern "C" int txrt_set_new_object(const char* type, const void* hash,
                                    const void* equal, void** result) noexcept
{
    return make_container<object_key_set>(type, hash, equal, result);
}

extern "C" int txrt_set_size_object(const void* value,
                                     std::int64_t* result) noexcept
{
    return container_apply<object_key_set>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_set_empty_object(const void* value, bool* result) noexcept
{
    return container_apply<object_key_set>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_set_clear_object(const void* value) noexcept
{
    return container_apply<object_key_set>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_set_contains_object(const void* value,
    const void* key, bool* result) noexcept
{
    return container_apply<object_key_set>(value, [&](auto& data)
    {
        *result = data.values.contains(*static_cast<const std::any*>(key));
    });
}

extern "C" int txrt_set_remove_object(const void* value,
    const void* key, bool* result) noexcept
{
    return container_apply<object_key_set>(value, [&](auto& data)
    {
        *result = data.values.erase(*static_cast<const std::any*>(key)) != 0;
    });
}

extern "C" int txrt_set_insert_object(const void* value,
    const void* key, bool* result) noexcept
{
    return container_apply<object_key_set>(value, [&](auto& data)
    {
        *result = data.insert(*static_cast<const std::any*>(key));
    });
}

extern "C" int txrt_set_to_vector_object(const void* value,
    void** result) noexcept
{
    return container_apply<object_key_set>(value, [&](auto& data)
    {
        container_result(data.snapshot(), result);
    });
}
