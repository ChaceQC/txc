#include "backend/cpp/vector_abi.hpp"
#include "backend/cpp/vector_abi_internal.hpp"
#include "stdlib/bytes.hpp"

namespace
{

tx_generated::byte_value element_value(const void* item)
{
    return item ? tx_generated::bytes_of(*static_cast<const std::any*>(item))
                : tx_generated::make_bytes({});
}

} // namespace

using namespace tx_generated::detail;

extern "C" void* txrt_vector_ref_bytes(const void* value) noexcept
{
    return vector_ref<tx_generated::byte_value>(value);
}

extern "C" int txrt_vector_new_bytes(std::int64_t count, const void* item,
                                      void** result) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::bytes_vector vector;
        vector.data().values.assign(vector_count(count), element_value(item));
        vector.data().refresh();
        *result = make_handle<std::any>(std::move(vector));
    });
}

extern "C" int txrt_vector_from_array_bytes(const void* value,
                                               void** result) noexcept
{
    return vector_from_array<tx_generated::byte_value>(value, result);
}

extern "C" int txrt_vector_to_array_bytes(const void* value,
                                             void** result) noexcept
{
    return vector_to_array<tx_generated::byte_value>(value, result);
}

extern "C" int txrt_vector_reserve_bytes(void* value,
                                           std::int64_t count) noexcept
{
    return vector_modify<tx_generated::byte_value>(value, [&](auto& values)
    {
        values.reserve(vector_count(count));
    });
}

extern "C" int txrt_vector_push_back_bytes(void* value,
                                             const void* item) noexcept
{
    return vector_modify<tx_generated::byte_value>(value, [&](auto& values)
    {
        values.push_back(element_value(item));
    });
}

extern "C" int txrt_vector_resize_bytes(void* value, std::int64_t count,
                                          const void* item) noexcept
{
    return vector_modify<tx_generated::byte_value>(value, [&](auto& values)
    {
        values.resize(vector_count(count), element_value(item));
    });
}

extern "C" int txrt_vector_clear_bytes(void* value) noexcept
{
    return vector_modify<tx_generated::byte_value>(value, [&](auto& values)
    {
        values.clear();
    });
}

extern "C" int txrt_vector_pop_back_bytes(void* value) noexcept
{
    return vector_modify<tx_generated::byte_value>(value, [&](auto& values)
    {
        if (values.empty())
        {
            throw std::out_of_range("空 vector 不能 pop_back");
        }
        values.pop_back();
    });
}

extern "C" int txrt_vector_insert_bytes(void* value, std::int64_t index,
                                          const void* item) noexcept
{
    return vector_modify<tx_generated::byte_value>(value, [&](auto& values)
    {
        const auto at = vector_count(index);
        if (at > values.size())
        {
            throw std::out_of_range("vector 插入位置越界");
        }
        values.insert(values.begin() + at, element_value(item));
    });
}

extern "C" int txrt_vector_erase_bytes(void* value,
                                         std::int64_t index) noexcept
{
    return vector_modify<tx_generated::byte_value>(value, [&](auto& values)
    {
        const auto at = vector_count(index);
        if (at >= values.size())
        {
            throw std::out_of_range("vector 删除位置越界");
        }
        values.erase(values.begin() + at);
    });
}

extern "C" int txrt_vector_get_bytes(const void* value, std::int64_t index,
                                       void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& values = vector_value<tx_generated::byte_value>(
            value).data().values;
        const auto at = vector_count(index);
        if (at >= values.size())
        {
            throw std::out_of_range("vector 索引越界");
        }
        *result = make_handle<std::any>(values[at]);
    });
}

extern "C" int txrt_vector_set_bytes(void* value, std::int64_t index,
                                       const void* item) noexcept
{
    return vector_modify<tx_generated::byte_value>(value, [&](auto& values)
    {
        const auto at = vector_count(index);
        if (at >= values.size())
        {
            throw std::out_of_range("vector 索引越界");
        }
        values[at] = element_value(item);
    });
}
