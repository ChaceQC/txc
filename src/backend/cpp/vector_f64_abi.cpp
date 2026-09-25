#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/vector_abi.hpp"
#include "backend/cpp/vector_abi_internal.hpp"

using namespace tx_generated::detail;

extern "C" void* txrt_vector_ref_f64(const void* value) noexcept
{
    return vector_ref<double>(value);
}

extern "C" int txrt_vector_new_f64(std::int64_t count, double item, void** result) noexcept
{
    return vector_new<double>(count, item, result);
}

extern "C" int txrt_vector_from_array_f64(const void* value, void** result) noexcept
{
    return vector_from_array<double>(value, result);
}

extern "C" int txrt_vector_to_array_f64(const void* value, void** result) noexcept
{
    return vector_to_array<double>(value, result);
}

extern "C" int txrt_vector_reserve_f64(void* value, std::int64_t count) noexcept
{
    return vector_modify<double>(value, [&](auto& values)
    {
        values.reserve(vector_count(count));
    });
}

extern "C" int txrt_vector_push_back_f64(void* value, double item) noexcept
{
    return vector_modify<double>(value, [&](auto& values)
    {
        values.push_back(item);
    });
}

extern "C" int txrt_vector_resize_f64(void* value, std::int64_t count, double item) noexcept
{
    return vector_modify<double>(value, [&](auto& values)
    {
        values.resize(vector_count(count), item);
    });
}

extern "C" int txrt_vector_clear_f64(void* value) noexcept
{
    return vector_modify<double>(value, [&](auto& values)
    {
        values.clear();
    });
}

extern "C" int txrt_vector_pop_back_f64(void* value) noexcept
{
    return vector_modify<double>(value, [&](auto& values)
    {
        if (values.empty())
        {
            throw std::out_of_range("空 vector 不能 pop_back");
        }
        values.pop_back();
    });
}

extern "C" int txrt_vector_insert_f64(void* value, std::int64_t index, double item) noexcept
{
    return vector_modify<double>(value, [&](auto& values)
    {
        const auto at = vector_count(index);
        if (at > values.size())
        {
            throw std::out_of_range("vector 插入位置越界");
        }
        values.insert(values.begin() + at, item);
    });
}

extern "C" int txrt_vector_erase_f64(void* value, std::int64_t index) noexcept
{
    return vector_modify<double>(value, [&](auto& values)
    {
        const auto at = vector_count(index);
        if (at >= values.size())
        {
            throw std::out_of_range("vector 删除位置越界");
        }
        values.erase(values.begin() + at);
    });
}
