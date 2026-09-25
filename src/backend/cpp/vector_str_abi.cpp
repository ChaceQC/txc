#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/vector_abi.hpp"
#include "backend/cpp/vector_abi_internal.hpp"

using namespace tx_generated::detail;

extern "C" void* txrt_vector_ref_str(const void* value) noexcept
{
    return vector_ref<tx_generated::text_reference>(value);
}

extern "C" int txrt_vector_new_str(std::int64_t count, const void* item, void** result) noexcept
{
    return vector_new<tx_generated::text_reference>(count, tx_generated::text_reference(item), result);
}

extern "C" int txrt_vector_from_array_str(const void* value, void** result) noexcept
{
    return vector_from_array<tx_generated::text_reference>(value, result);
}

extern "C" int txrt_vector_to_array_str(const void* value, void** result) noexcept
{
    return vector_to_array<tx_generated::text_reference>(value, result);
}

extern "C" int txrt_vector_reserve_str(void* value, std::int64_t count) noexcept
{
    return vector_modify<tx_generated::text_reference>(value, [&](auto& values)
    {
        values.reserve(vector_count(count));
    });
}

extern "C" int txrt_vector_push_back_str(void* value, const void* item) noexcept
{
    return vector_modify<tx_generated::text_reference>(value, [&](auto& values)
    {
        values.push_back(tx_generated::text_reference(item));
    });
}

extern "C" int txrt_vector_resize_str(void* value, std::int64_t count, const void* item) noexcept
{
    return vector_modify<tx_generated::text_reference>(value, [&](auto& values)
    {
        values.resize(vector_count(count), tx_generated::text_reference(item));
    });
}

extern "C" int txrt_vector_clear_str(void* value) noexcept
{
    return vector_modify<tx_generated::text_reference>(value, [&](auto& values)
    {
        values.clear();
    });
}

extern "C" int txrt_vector_pop_back_str(void* value) noexcept
{
    return vector_modify<tx_generated::text_reference>(value, [&](auto& values)
    {
        if (values.empty())
        {
            throw std::out_of_range("空 vector 不能 pop_back");
        }
        values.pop_back();
    });
}

extern "C" int txrt_vector_insert_str(void* value, std::int64_t index, const void* item) noexcept
{
    return vector_modify<tx_generated::text_reference>(value, [&](auto& values)
    {
        const auto at = vector_count(index);
        if (at > values.size())
        {
            throw std::out_of_range("vector 插入位置越界");
        }
        values.insert(values.begin() + at, tx_generated::text_reference(item));
    });
}

extern "C" int txrt_vector_erase_str(void* value, std::int64_t index) noexcept
{
    return vector_modify<tx_generated::text_reference>(value, [&](auto& values)
    {
        const auto at = vector_count(index);
        if (at >= values.size())
        {
            throw std::out_of_range("vector 删除位置越界");
        }
        values.erase(values.begin() + at);
    });
}
