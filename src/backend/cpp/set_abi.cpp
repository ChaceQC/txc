#include "backend/cpp/typed_container_abi.hpp"
#include "backend/cpp/container_abi_internal.hpp"
#include "stdlib/typed_map.hpp"

using namespace tx_generated;
using namespace tx_generated::detail;

extern "C" int txrt_set_new_i64(void** result) noexcept
{
    return container_new<set_storage<std::int64_t>>(result);
}

extern "C" int txrt_set_size_i64(const void* value, std::int64_t* result) noexcept
{
    return container_apply<set_storage<std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_set_empty_i64(const void* value, bool* result) noexcept
{
    return container_apply<set_storage<std::int64_t>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_set_clear_i64(const void* value) noexcept
{
    return container_apply<set_storage<std::int64_t>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_set_contains_i64(const void* value, std::int64_t key, bool* result) noexcept
{
    return container_apply<set_storage<std::int64_t>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_set_remove_i64(const void* value, std::int64_t key, bool* result) noexcept
{
    return container_apply<set_storage<std::int64_t>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_set_to_vector_i64(const void* value, void** result) noexcept
{
    return container_apply<set_storage<std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.snapshot(), result);
    });
}

extern "C" int txrt_set_insert_i64(const void* value, std::int64_t item, bool* result) noexcept
{
    return container_apply<set_storage<std::int64_t>>(value, [&](auto& data)
    {
        *result = data.insert(item);
    });
}

extern "C" int txrt_set_new_f64(void** result) noexcept
{
    return container_new<set_storage<double>>(result);
}

extern "C" int txrt_set_size_f64(const void* value, std::int64_t* result) noexcept
{
    return container_apply<set_storage<double>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_set_empty_f64(const void* value, bool* result) noexcept
{
    return container_apply<set_storage<double>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_set_clear_f64(const void* value) noexcept
{
    return container_apply<set_storage<double>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_set_contains_f64(const void* value, double key, bool* result) noexcept
{
    return container_apply<set_storage<double>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_set_remove_f64(const void* value, double key, bool* result) noexcept
{
    return container_apply<set_storage<double>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_set_to_vector_f64(const void* value, void** result) noexcept
{
    return container_apply<set_storage<double>>(value, [&](auto& data)
    {
        container_result(data.snapshot(), result);
    });
}

extern "C" int txrt_set_insert_f64(const void* value, double item, bool* result) noexcept
{
    return container_apply<set_storage<double>>(value, [&](auto& data)
    {
        *result = data.insert(item);
    });
}

extern "C" int txrt_set_new_bool(void** result) noexcept
{
    return container_new<set_storage<std::uint8_t>>(result);
}

extern "C" int txrt_set_size_bool(const void* value, std::int64_t* result) noexcept
{
    return container_apply<set_storage<std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_set_empty_bool(const void* value, bool* result) noexcept
{
    return container_apply<set_storage<std::uint8_t>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_set_clear_bool(const void* value) noexcept
{
    return container_apply<set_storage<std::uint8_t>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_set_contains_bool(const void* value, bool key, bool* result) noexcept
{
    return container_apply<set_storage<std::uint8_t>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_set_remove_bool(const void* value, bool key, bool* result) noexcept
{
    return container_apply<set_storage<std::uint8_t>>(value, [&](auto& data)
    {
        const auto item = key;
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_set_to_vector_bool(const void* value, void** result) noexcept
{
    return container_apply<set_storage<std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.snapshot(), result);
    });
}

extern "C" int txrt_set_insert_bool(const void* value, bool item, bool* result) noexcept
{
    return container_apply<set_storage<std::uint8_t>>(value, [&](auto& data)
    {
        *result = data.insert(item);
    });
}

extern "C" int txrt_set_new_str(void** result) noexcept
{
    return container_new<set_storage<text_reference>>(result);
}

extern "C" int txrt_set_size_str(const void* value, std::int64_t* result) noexcept
{
    return container_apply<set_storage<text_reference>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_set_empty_str(const void* value, bool* result) noexcept
{
    return container_apply<set_storage<text_reference>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_set_clear_str(const void* value) noexcept
{
    return container_apply<set_storage<text_reference>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_set_contains_str(const void* value, const void* key, bool* result) noexcept
{
    return container_apply<set_storage<text_reference>>(value, [&](auto& data)
    {
        const auto item = text_reference(key);
        require_ordered_key(item);
        *result = data.values.contains(item);
    });
}

extern "C" int txrt_set_remove_str(const void* value, const void* key, bool* result) noexcept
{
    return container_apply<set_storage<text_reference>>(value, [&](auto& data)
    {
        const auto item = text_reference(key);
        require_ordered_key(item);
        *result = data.values.erase(item) != 0;
    });
}

extern "C" int txrt_set_to_vector_str(const void* value, void** result) noexcept
{
    return container_apply<set_storage<text_reference>>(value, [&](auto& data)
    {
        container_result(data.snapshot(), result);
    });
}

extern "C" int txrt_set_insert_str(const void* value, const void* item, bool* result) noexcept
{
    return container_apply<set_storage<text_reference>>(value, [&](auto& data)
    {
        *result = data.insert(text_reference(item));
    });
}
