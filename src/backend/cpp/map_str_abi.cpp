#include "backend/cpp/typed_container_abi.hpp"
#include "backend/cpp/container_abi_internal.hpp"
#include "stdlib/typed_map.hpp"

using namespace tx_generated;
using namespace tx_generated::detail;

extern "C" int txrt_map_new_str_i64(void** result) noexcept
{
    return container_new<map_storage<text_reference, std::int64_t>>(result);
}

extern "C" int txrt_map_size_str_i64(const void* value, std::int64_t* result) noexcept
{
    return container_apply<map_storage<text_reference, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_map_empty_str_i64(const void* value, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, std::int64_t>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_map_clear_str_i64(const void* value) noexcept
{
    return container_apply<map_storage<text_reference, std::int64_t>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_map_contains_str_i64(const void* value, const void* key, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, std::int64_t>>(value, [&](auto& data)
    {
        const auto item = text_reference(key);
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_map_remove_str_i64(const void* value, const void* key, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, std::int64_t>>(value, [&](auto& data)
    {
        const auto item = text_reference(key);
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_map_set_str_i64(const void* value, const void* key, std::int64_t item) noexcept
{
    return container_apply<map_storage<text_reference, std::int64_t>>(value, [&](auto& data)
    {
        data.set(text_reference(key), item);
    });
}

extern "C" int txrt_map_read_str_i64(const void* value, const void* key, std::int64_t* result) noexcept
{
    return container_apply<map_storage<text_reference, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.read(text_reference(key)), result);
    });
}

extern "C" int txrt_map_get_str_i64(const void* value, const void* key, std::int64_t fallback, std::int64_t* result) noexcept
{
    return container_apply<map_storage<text_reference, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.get(text_reference(key), fallback), result);
    });
}

extern "C" int txrt_map_keys_str_i64(const void* value, void** result) noexcept
{
    return container_apply<map_storage<text_reference, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<true>(), result);
    });
}

extern "C" int txrt_map_values_str_i64(const void* value, void** result) noexcept
{
    return container_apply<map_storage<text_reference, std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<false>(), result);
    });
}

extern "C" int txrt_map_new_str_f64(void** result) noexcept
{
    return container_new<map_storage<text_reference, double>>(result);
}

extern "C" int txrt_map_size_str_f64(const void* value, std::int64_t* result) noexcept
{
    return container_apply<map_storage<text_reference, double>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_map_empty_str_f64(const void* value, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, double>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_map_clear_str_f64(const void* value) noexcept
{
    return container_apply<map_storage<text_reference, double>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_map_contains_str_f64(const void* value, const void* key, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, double>>(value, [&](auto& data)
    {
        const auto item = text_reference(key);
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_map_remove_str_f64(const void* value, const void* key, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, double>>(value, [&](auto& data)
    {
        const auto item = text_reference(key);
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_map_set_str_f64(const void* value, const void* key, double item) noexcept
{
    return container_apply<map_storage<text_reference, double>>(value, [&](auto& data)
    {
        data.set(text_reference(key), item);
    });
}

extern "C" int txrt_map_read_str_f64(const void* value, const void* key, double* result) noexcept
{
    return container_apply<map_storage<text_reference, double>>(value, [&](auto& data)
    {
        container_result(data.read(text_reference(key)), result);
    });
}

extern "C" int txrt_map_get_str_f64(const void* value, const void* key, double fallback, double* result) noexcept
{
    return container_apply<map_storage<text_reference, double>>(value, [&](auto& data)
    {
        container_result(data.get(text_reference(key), fallback), result);
    });
}

extern "C" int txrt_map_keys_str_f64(const void* value, void** result) noexcept
{
    return container_apply<map_storage<text_reference, double>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<true>(), result);
    });
}

extern "C" int txrt_map_values_str_f64(const void* value, void** result) noexcept
{
    return container_apply<map_storage<text_reference, double>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<false>(), result);
    });
}

extern "C" int txrt_map_new_str_bool(void** result) noexcept
{
    return container_new<map_storage<text_reference, std::uint8_t>>(result);
}

extern "C" int txrt_map_size_str_bool(const void* value, std::int64_t* result) noexcept
{
    return container_apply<map_storage<text_reference, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_map_empty_str_bool(const void* value, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, std::uint8_t>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_map_clear_str_bool(const void* value) noexcept
{
    return container_apply<map_storage<text_reference, std::uint8_t>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_map_contains_str_bool(const void* value, const void* key, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, std::uint8_t>>(value, [&](auto& data)
    {
        const auto item = text_reference(key);
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_map_remove_str_bool(const void* value, const void* key, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, std::uint8_t>>(value, [&](auto& data)
    {
        const auto item = text_reference(key);
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_map_set_str_bool(const void* value, const void* key, bool item) noexcept
{
    return container_apply<map_storage<text_reference, std::uint8_t>>(value, [&](auto& data)
    {
        data.set(text_reference(key), item);
    });
}

extern "C" int txrt_map_read_str_bool(const void* value, const void* key, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.read(text_reference(key)), result);
    });
}

extern "C" int txrt_map_get_str_bool(const void* value, const void* key, bool fallback, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.get(text_reference(key), fallback), result);
    });
}

extern "C" int txrt_map_keys_str_bool(const void* value, void** result) noexcept
{
    return container_apply<map_storage<text_reference, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<true>(), result);
    });
}

extern "C" int txrt_map_values_str_bool(const void* value, void** result) noexcept
{
    return container_apply<map_storage<text_reference, std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<false>(), result);
    });
}

extern "C" int txrt_map_new_str_str(void** result) noexcept
{
    return container_new<map_storage<text_reference, text_reference>>(result);
}

extern "C" int txrt_map_size_str_str(const void* value, std::int64_t* result) noexcept
{
    return container_apply<map_storage<text_reference, text_reference>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_map_empty_str_str(const void* value, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, text_reference>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_map_clear_str_str(const void* value) noexcept
{
    return container_apply<map_storage<text_reference, text_reference>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_map_contains_str_str(const void* value, const void* key, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, text_reference>>(value, [&](auto& data)
    {
        const auto item = text_reference(key);
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_map_remove_str_str(const void* value, const void* key, bool* result) noexcept
{
    return container_apply<map_storage<text_reference, text_reference>>(value, [&](auto& data)
    {
        const auto item = text_reference(key);
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_map_set_str_str(const void* value, const void* key, const void* item) noexcept
{
    return container_apply<map_storage<text_reference, text_reference>>(value, [&](auto& data)
    {
        data.set(text_reference(key), text_reference(item));
    });
}

extern "C" int txrt_map_read_str_str(const void* value, const void* key, void** result) noexcept
{
    return container_apply<map_storage<text_reference, text_reference>>(value, [&](auto& data)
    {
        container_result(data.read(text_reference(key)), result);
    });
}

extern "C" int txrt_map_get_str_str(const void* value, const void* key, const void* fallback, void** result) noexcept
{
    return container_apply<map_storage<text_reference, text_reference>>(value, [&](auto& data)
    {
        container_result(data.get(text_reference(key), text_reference(fallback)), result);
    });
}

extern "C" int txrt_map_keys_str_str(const void* value, void** result) noexcept
{
    return container_apply<map_storage<text_reference, text_reference>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<true>(), result);
    });
}

extern "C" int txrt_map_values_str_str(const void* value, void** result) noexcept
{
    return container_apply<map_storage<text_reference, text_reference>>(value, [&](auto& data)
    {
        container_result(data.template snapshot<false>(), result);
    });
}
