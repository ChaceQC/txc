#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/vector_abi.hpp"
#include "backend/cpp/vector_abi_internal.hpp"

using namespace tx_generated::detail;

extern "C" void* txrt_vector_ref_bool(const void* value) noexcept
{
    return vector_ref<std::uint8_t>(value);
}

extern "C" int txrt_vector_new_bool(std::int64_t count, bool item, void** result) noexcept
{
    return vector_new<std::uint8_t>(count, static_cast<std::uint8_t>(item), result);
}

extern "C" int txrt_vector_from_array_bool(const void* value, void** result) noexcept
{
    return vector_from_array<std::uint8_t>(value, result);
}

extern "C" int txrt_vector_to_array_bool(const void* value, void** result) noexcept
{
    return vector_to_array<std::uint8_t>(value, result);
}

extern "C" int txrt_vector_reserve_bool(void* value, std::int64_t count) noexcept
{
    return vector_modify<std::uint8_t>(value, [&](auto& values)
    {
        values.reserve(vector_count(count));
    });
}

extern "C" int txrt_vector_push_back_bool(void* value, bool item) noexcept
{
    return vector_modify<std::uint8_t>(value, [&](auto& values)
    {
        values.push_back(static_cast<std::uint8_t>(item));
    });
}

extern "C" int txrt_vector_resize_bool(void* value, std::int64_t count, bool item) noexcept
{
    return vector_modify<std::uint8_t>(value, [&](auto& values)
    {
        values.resize(vector_count(count), static_cast<std::uint8_t>(item));
    });
}

extern "C" int txrt_vector_clear_bool(void* value) noexcept
{
    return vector_modify<std::uint8_t>(value, [&](auto& values)
    {
        values.clear();
    });
}

extern "C" int txrt_vector_pop_back_bool(void* value) noexcept
{
    return vector_modify<std::uint8_t>(value, [&](auto& values)
    {
        if (values.empty())
        {
            throw std::out_of_range("空 vector 不能 pop_back");
        }
        values.pop_back();
    });
}

extern "C" int txrt_vector_insert_bool(void* value, std::int64_t index, bool item) noexcept
{
    return vector_modify<std::uint8_t>(value, [&](auto& values)
    {
        const auto at = vector_count(index);
        if (at > values.size())
        {
            throw std::out_of_range("vector 插入位置越界");
        }
        values.insert(values.begin() + at, static_cast<std::uint8_t>(item));
    });
}

extern "C" int txrt_vector_erase_bool(void* value, std::int64_t index) noexcept
{
    return vector_modify<std::uint8_t>(value, [&](auto& values)
    {
        const auto at = vector_count(index);
        if (at >= values.size())
        {
            throw std::out_of_range("vector 删除位置越界");
        }
        values.erase(values.begin() + at);
    });
}
