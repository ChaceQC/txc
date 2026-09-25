#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/vector_abi.hpp"
#include "backend/cpp/vector_abi_internal.hpp"

using namespace tx_generated::detail;

extern "C" void* txrt_vector_ref_i64(const void* value) noexcept
{
    return vector_ref<std::int64_t>(value);
}

extern "C" int txrt_vector_new_i64(std::int64_t count, std::int64_t item, void** result) noexcept
{
    return vector_new<std::int64_t>(count, item, result);
}

extern "C" int txrt_vector_from_array_i64(const void* value, void** result) noexcept
{
    return vector_from_array<std::int64_t>(value, result);
}

extern "C" int txrt_vector_to_array_i64(const void* value, void** result) noexcept
{
    return vector_to_array<std::int64_t>(value, result);
}

extern "C" int txrt_vector_reserve_i64(void* value, std::int64_t count) noexcept
{
    return vector_modify<std::int64_t>(value, [&](auto& values)
    {
        values.reserve(vector_count(count));
    });
}

extern "C" int txrt_vector_push_back_i64(void* value, std::int64_t item) noexcept
{
    return vector_modify<std::int64_t>(value, [&](auto& values)
    {
        values.push_back(item);
    });
}

extern "C" int txrt_vector_resize_i64(void* value, std::int64_t count, std::int64_t item) noexcept
{
    return vector_modify<std::int64_t>(value, [&](auto& values)
    {
        values.resize(vector_count(count), item);
    });
}

extern "C" int txrt_vector_clear_i64(void* value) noexcept
{
    return vector_modify<std::int64_t>(value, [&](auto& values)
    {
        values.clear();
    });
}

extern "C" int txrt_vector_pop_back_i64(void* value) noexcept
{
    return vector_modify<std::int64_t>(value, [&](auto& values)
    {
        if (values.empty())
        {
            throw std::out_of_range("空 vector 不能 pop_back");
        }
        values.pop_back();
    });
}

extern "C" int txrt_vector_insert_i64(void* value, std::int64_t index, std::int64_t item) noexcept
{
    return vector_modify<std::int64_t>(value, [&](auto& values)
    {
        const auto at = vector_count(index);
        if (at > values.size())
        {
            throw std::out_of_range("vector 插入位置越界");
        }
        values.insert(values.begin() + at, item);
    });
}

extern "C" int txrt_vector_erase_i64(void* value, std::int64_t index) noexcept
{
    return vector_modify<std::int64_t>(value, [&](auto& values)
    {
        const auto at = vector_count(index);
        if (at >= values.size())
        {
            throw std::out_of_range("vector 删除位置越界");
        }
        values.erase(values.begin() + at);
    });
}
