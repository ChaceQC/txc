#include "backend/cpp/vector_abi.hpp"
#include "backend/cpp/vector_abi_internal.hpp"
#include "backend/cpp/value_abi.hpp"

#include <any>
#include <stdexcept>
#include <string>

namespace
{

using tx_generated::object_vector;
using tx_generated::detail::vector_count;
using tx_generated::detail::vector_value;

void require_element(const std::any& item, const std::string& type)
{
    if (txrt_value_require_type(&item, type.c_str()) != 0)
    {
        throw std::runtime_error("array 元素与 vector<" + type + "> 的元素类型不匹配");
    }
}

object_vector& checked_vector(const void* value)
{
    return vector_value<std::any>(value);
}

std::any checked_element(const object_vector& vector, const void* item)
{
    (void)vector;
    if (!item)
    {
        throw std::runtime_error("复合 vector 元素不能为 none");
    }
    // 普通写入的 T 已在语义分析确定；仅 array 转换属于动态边界。
    return *static_cast<const std::any*>(item);
}

} // namespace

using namespace tx_generated::detail;

extern "C" void* txrt_vector_ref_object(const void* value) noexcept
{
    return vector_ref<std::any>(value);
}

extern "C" int txrt_vector_new_object(std::int64_t count, const void* item,
                                       const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        object_vector vector(type_name);
        const auto size = vector_count(count);
        if (size != 0)
        {
            vector.data().values.assign(size, checked_element(vector, item));
        }
        vector.data().refresh();
        *result = make_handle<std::any>(std::move(vector));
    });
}

extern "C" int txrt_vector_from_array_object(const void* value,
                                               const char* type_name,
                                               void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& source = std::any_cast<const tx_generated::tx_array&>(
            *static_cast<const std::any*>(value));
        object_vector vector(type_name);
        auto& values = vector.data().values;
        values.reserve(source.size());
        for (const auto& item : source)
        {
            require_element(item, vector.data().type_name);
            values.push_back(item);
        }
        vector.data().refresh();
        *result = make_handle<std::any>(std::move(vector));
    });
}

extern "C" int txrt_vector_to_array_object(const void* value,
                                             void** result) noexcept
{
    return vector_to_array<std::any>(value, result);
}

extern "C" int txrt_vector_reserve_object(void* value,
                                           std::int64_t count) noexcept
{
    return vector_modify<std::any>(value, [&](auto& values)
    {
        values.reserve(vector_count(count));
    });
}

extern "C" int txrt_vector_push_back_object(void* value,
                                             const void* item) noexcept
{
    return invoke_checked([&]
    {
        auto& vector = checked_vector(value);
        auto element = checked_element(vector, item);
        auto& values = vector.data().values;
        values.push_back(std::move(element));
        vector.data().refresh();
    });
}

extern "C" int txrt_vector_resize_object(void* value, std::int64_t count,
                                          const void* item) noexcept
{
    return invoke_checked([&]
    {
        auto& vector = checked_vector(value);
        const auto size = vector_count(count);
        auto& values = vector.data().values;
        if (size > values.size())
        {
            auto element = checked_element(vector, item);
            values.resize(size, element);
            vector.data().refresh();
        }
        else
        {
            std::vector<std::any> removed;
            removed.reserve(values.size() - size);
            for (std::size_t index = size; index < values.size(); ++index)
            {
                removed.push_back(std::move(values[index]));
            }
            values.resize(size);
            vector.data().refresh();
        }
    });
}

extern "C" int txrt_vector_clear_object(void* value) noexcept
{
    return invoke_checked([&]
    {
        auto& vector = checked_vector(value);
        std::vector<std::any> removed;
        removed.swap(vector.data().values);
        vector.data().refresh();
    });
}

extern "C" int txrt_vector_pop_back_object(void* value) noexcept
{
    return invoke_checked([&]
    {
        auto& vector = checked_vector(value);
        auto& values = vector.data().values;
        if (values.empty())
        {
            throw std::out_of_range("空 vector 不能 pop_back");
        }
        auto removed = std::move(values.back());
        values.pop_back();
        vector.data().refresh();
    });
}

extern "C" int txrt_vector_insert_object(void* value, std::int64_t index,
                                          const void* item) noexcept
{
    return invoke_checked([&]
    {
        auto& vector = checked_vector(value);
        const auto at = vector_count(index);
        auto& values = vector.data().values;
        if (at > values.size())
        {
            throw std::out_of_range("vector 插入位置越界");
        }
        auto element = checked_element(vector, item);
        values.insert(values.begin() + at, std::move(element));
        vector.data().refresh();
    });
}

extern "C" int txrt_vector_erase_object(void* value,
                                         std::int64_t index) noexcept
{
    return invoke_checked([&]
    {
        auto& vector = checked_vector(value);
        const auto at = vector_count(index);
        auto& values = vector.data().values;
        if (at >= values.size())
        {
            throw std::out_of_range("vector 删除位置越界");
        }
        auto removed = std::move(values[at]);
        values.erase(values.begin() + at);
        vector.data().refresh();
    });
}

extern "C" int txrt_vector_get_object(const void* value, std::int64_t index,
                                       void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto& values = checked_vector(value).data().values;
        const auto at = vector_count(index);
        if (at >= values.size())
        {
            throw std::out_of_range("vector 索引越界");
        }
        *result = make_handle<std::any>(values[at]);
    });
}

extern "C" void* txrt_vector_element_address_object(void* value,
                                                      std::int64_t index) noexcept
{
    void* result = nullptr;
    txrt_require_success(invoke_checked([&]
    {
        auto& values = checked_vector(value).data().values;
        const auto at = vector_count(index);
        if (at >= values.size())
        {
            throw std::out_of_range("vector 索引越界");
        }
        result = &values[at];
    }));
    return result;
}

extern "C" int txrt_vector_set_object(void* value, std::int64_t index,
                                       const void* item) noexcept
{
    return invoke_checked([&]
    {
        auto& vector = checked_vector(value);
        const auto at = vector_count(index);
        auto& values = vector.data().values;
        if (at >= values.size())
        {
            throw std::out_of_range("vector 索引越界");
        }
        auto element = checked_element(vector, item);
        auto removed = std::move(values[at]);
        values[at] = std::move(element);
    });
}
