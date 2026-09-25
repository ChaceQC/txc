#include "backend/cpp/typed_container_abi.hpp"
#include "backend/cpp/container_abi_internal.hpp"
#include "stdlib/typed_map.hpp"

using namespace tx_generated;
using namespace tx_generated::detail;

extern "C" int txrt_map_new_bool_i64(void** result) noexcept
{
    return container_new<map_storage<std::uint8_t, std::int64_t>>(result);
}

extern "C" int txrt_map_size_bool_i64(const void* value, std::int64_t* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_map_empty_bool_i64(const void* value, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::int64_t>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_map_clear_bool_i64(const void* value) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::int64_t>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_map_contains_bool_i64(const void* value, bool key, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::int64_t>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_map_remove_bool_i64(const void* value, bool key, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::int64_t>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_map_set_bool_i64(const void* value, bool key, std::int64_t item) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::int64_t>>(value, [&](auto& data)
    {
        data.set(key, item);
    });
}

extern "C" int txrt_map_read_bool_i64(const void* value, bool key, std::int64_t* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.read(key), result);
    });
}

extern "C" int txrt_map_get_bool_i64(const void* value, bool key, std::int64_t fallback, std::int64_t* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.get(key, fallback), result);
    });
}

extern "C" int txrt_map_keys_bool_i64(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<true>(), result);
    });
}

extern "C" int txrt_map_values_bool_i64(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<false>(), result);
    });
}

extern "C" int txrt_map_new_bool_f64(void** result) noexcept
{
    return container_new<map_storage<std::uint8_t, double>>(result);
}

extern "C" int txrt_map_size_bool_f64(const void* value, std::int64_t* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, double>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_map_empty_bool_f64(const void* value, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, double>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_map_clear_bool_f64(const void* value) noexcept
{
    return container_apply<map_storage<std::uint8_t, double>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_map_contains_bool_f64(const void* value, bool key, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, double>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_map_remove_bool_f64(const void* value, bool key, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, double>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_map_set_bool_f64(const void* value, bool key, double item) noexcept
{
    return container_apply<map_storage<std::uint8_t, double>>(value, [&](auto& data)
    {
        data.set(key, item);
    });
}

extern "C" int txrt_map_read_bool_f64(const void* value, bool key, double* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, double>>(value, [&](auto& data)
    {
        container_result(data.read(key), result);
    });
}

extern "C" int txrt_map_get_bool_f64(const void* value, bool key, double fallback, double* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, double>>(value, [&](auto& data)
    {
        container_result(data.get(key, fallback), result);
    });
}

extern "C" int txrt_map_keys_bool_f64(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::uint8_t, double>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<true>(), result);
    });
}

extern "C" int txrt_map_values_bool_f64(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::uint8_t, double>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<false>(), result);
    });
}

extern "C" int txrt_map_new_bool_bool(void** result) noexcept
{
    return container_new<map_storage<std::uint8_t, std::uint8_t>>(result);
}

extern "C" int txrt_map_size_bool_bool(const void* value, std::int64_t* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_map_empty_bool_bool(const void* value, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::uint8_t>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_map_clear_bool_bool(const void* value) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::uint8_t>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_map_contains_bool_bool(const void* value, bool key, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::uint8_t>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_map_remove_bool_bool(const void* value, bool key, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::uint8_t>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_map_set_bool_bool(const void* value, bool key, bool item) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::uint8_t>>(value, [&](auto& data)
    {
        data.set(key, item);
    });
}

extern "C" int txrt_map_read_bool_bool(const void* value, bool key, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.read(key), result);
    });
}

extern "C" int txrt_map_get_bool_bool(const void* value, bool key, bool fallback, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.get(key, fallback), result);
    });
}

extern "C" int txrt_map_keys_bool_bool(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<true>(), result);
    });
}

extern "C" int txrt_map_values_bool_bool(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::uint8_t, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<false>(), result);
    });
}

extern "C" int txrt_map_new_bool_str(void** result) noexcept
{
    return container_new<map_storage<std::uint8_t, text_reference>>(result);
}

extern "C" int txrt_map_size_bool_str(const void* value, std::int64_t* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, text_reference>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_map_empty_bool_str(const void* value, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, text_reference>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_map_clear_bool_str(const void* value) noexcept
{
    return container_apply<map_storage<std::uint8_t, text_reference>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_map_contains_bool_str(const void* value, bool key, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, text_reference>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_map_remove_bool_str(const void* value, bool key, bool* result) noexcept
{
    return container_apply<map_storage<std::uint8_t, text_reference>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_map_set_bool_str(const void* value, bool key, const void* item) noexcept
{
    return container_apply<map_storage<std::uint8_t, text_reference>>(value, [&](auto& data)
    {
        data.set(key, text_reference(item));
    });
}

extern "C" int txrt_map_read_bool_str(const void* value, bool key, void** result) noexcept
{
    return container_apply<map_storage<std::uint8_t, text_reference>>(value, [&](auto& data)
    {
        container_result(data.read(key), result);
    });
}

extern "C" int txrt_map_get_bool_str(const void* value, bool key, const void* fallback, void** result) noexcept
{
    return container_apply<map_storage<std::uint8_t, text_reference>>(value, [&](auto& data)
    {
        container_result(data.get(key, text_reference(fallback)), result);
    });
}

extern "C" int txrt_map_keys_bool_str(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::uint8_t, text_reference>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<true>(), result);
    });
}

extern "C" int txrt_map_values_bool_str(const void* value, void** result) noexcept
{
    return container_apply<map_storage<std::uint8_t, text_reference>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<false>(), result);
    });
}
