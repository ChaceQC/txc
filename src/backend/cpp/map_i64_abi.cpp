#include "backend/cpp/typed_container_abi.hpp"
#include "backend/cpp/container_abi_internal.hpp"
#include "stdlib/typed_map.hpp"

using namespace tx_generated;
using namespace tx_generated::detail;

extern "C" int txrt_map_new_i64_i64(void** result) noexcept
{
    return container_new<map_storage<std::int64_t, std::int64_t>>(result);
}

extern "C" int txrt_map_size_i64_i64(const void* value, std::int64_t* result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_map_empty_i64_i64(const void* value, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::int64_t>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_map_clear_i64_i64(const void* value) noexcept
{
    return container_apply<map_storage<std::int64_t, std::int64_t>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_map_contains_i64_i64(const void* value, std::int64_t key, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::int64_t>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_map_remove_i64_i64(const void* value, std::int64_t key, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::int64_t>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_map_set_i64_i64(const void* value, std::int64_t key, std::int64_t item) noexcept
{
    return container_apply<map_storage<std::int64_t, std::int64_t>>(value, [&](auto& data)
    {
        data.set(key, item);
    });
}

extern "C" int txrt_map_read_i64_i64(const void* value, std::int64_t key, std::int64_t* result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.read(key), result);
    });
}

extern "C" int txrt_map_get_i64_i64(const void* value, std::int64_t key, std::int64_t fallback, std::int64_t* result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.get(key, fallback), result);
    });
}

extern "C" int txrt_map_keys_i64_i64(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<true>(), result);
    });
}

extern "C" int txrt_map_values_i64_i64(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<false>(), result);
    });
}

extern "C" int txrt_map_new_i64_f64(void** result) noexcept
{
    return container_new<map_storage<std::int64_t, double>>(result);
}

extern "C" int txrt_map_size_i64_f64(const void* value, std::int64_t* result) noexcept
{
    return container_apply<map_storage<std::int64_t, double>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_map_empty_i64_f64(const void* value, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, double>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_map_clear_i64_f64(const void* value) noexcept
{
    return container_apply<map_storage<std::int64_t, double>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_map_contains_i64_f64(const void* value, std::int64_t key, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, double>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_map_remove_i64_f64(const void* value, std::int64_t key, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, double>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_map_set_i64_f64(const void* value, std::int64_t key, double item) noexcept
{
    return container_apply<map_storage<std::int64_t, double>>(value, [&](auto& data)
    {
        data.set(key, item);
    });
}

extern "C" int txrt_map_read_i64_f64(const void* value, std::int64_t key, double* result) noexcept
{
    return container_apply<map_storage<std::int64_t, double>>(value, [&](auto& data)
    {
        container_result(data.read(key), result);
    });
}

extern "C" int txrt_map_get_i64_f64(const void* value, std::int64_t key, double fallback, double* result) noexcept
{
    return container_apply<map_storage<std::int64_t, double>>(value, [&](auto& data)
    {
        container_result(data.get(key, fallback), result);
    });
}

extern "C" int txrt_map_keys_i64_f64(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::int64_t, double>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<true>(), result);
    });
}

extern "C" int txrt_map_values_i64_f64(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::int64_t, double>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<false>(), result);
    });
}

extern "C" int txrt_map_new_i64_bool(void** result) noexcept
{
    return container_new<map_storage<std::int64_t, std::uint8_t>>(result);
}

extern "C" int txrt_map_size_i64_bool(const void* value, std::int64_t* result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_map_empty_i64_bool(const void* value, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::uint8_t>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_map_clear_i64_bool(const void* value) noexcept
{
    return container_apply<map_storage<std::int64_t, std::uint8_t>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_map_contains_i64_bool(const void* value, std::int64_t key, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::uint8_t>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_map_remove_i64_bool(const void* value, std::int64_t key, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::uint8_t>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_map_set_i64_bool(const void* value, std::int64_t key, bool item) noexcept
{
    return container_apply<map_storage<std::int64_t, std::uint8_t>>(value, [&](auto& data)
    {
        data.set(key, item);
    });
}

extern "C" int txrt_map_read_i64_bool(const void* value, std::int64_t key, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.read(key), result);
    });
}

extern "C" int txrt_map_get_i64_bool(const void* value, std::int64_t key, bool fallback, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.get(key, fallback), result);
    });
}

extern "C" int txrt_map_keys_i64_bool(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<true>(), result);
    });
}

extern "C" int txrt_map_values_i64_bool(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::int64_t, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<false>(), result);
    });
}

extern "C" int txrt_map_new_i64_str(void** result) noexcept
{
    return container_new<map_storage<std::int64_t, text_reference>>(result);
}

extern "C" int txrt_map_size_i64_str(const void* value, std::int64_t* result) noexcept
{
    return container_apply<map_storage<std::int64_t, text_reference>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_map_empty_i64_str(const void* value, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, text_reference>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_map_clear_i64_str(const void* value) noexcept
{
    return container_apply<map_storage<std::int64_t, text_reference>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_map_contains_i64_str(const void* value, std::int64_t key, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, text_reference>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_map_remove_i64_str(const void* value, std::int64_t key, bool* result) noexcept
{
    return container_apply<map_storage<std::int64_t, text_reference>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_map_set_i64_str(const void* value, std::int64_t key, const void* item) noexcept
{
    return container_apply<map_storage<std::int64_t, text_reference>>(value, [&](auto& data)
    {
        data.set(key, text_reference(item));
    });
}

extern "C" int txrt_map_read_i64_str(const void* value, std::int64_t key, void** result) noexcept
{
    return container_apply<map_storage<std::int64_t, text_reference>>(value, [&](auto& data)
    {
        container_result(data.read(key), result);
    });
}

extern "C" int txrt_map_get_i64_str(const void* value, std::int64_t key, const void* fallback, void** result) noexcept
{
    return container_apply<map_storage<std::int64_t, text_reference>>(value, [&](auto& data)
    {
        container_result(data.get(key, text_reference(fallback)), result);
    });
}

extern "C" int txrt_map_keys_i64_str(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::int64_t, text_reference>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<true>(), result);
    });
}

extern "C" int txrt_map_values_i64_str(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::int64_t, text_reference>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<false>(), result);
    });
}
