#include "backend/cpp/container_abi_internal.hpp"
#include "stdlib/typed_deque.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace
{

using namespace tx_generated;
using namespace tx_generated::detail;

std::int64_t stored_value(std::int64_t value)
{
    return value;
}

double stored_value(double value)
{
    return value;
}

std::uint8_t stored_value(bool value)
{
    return value;
}

text_reference stored_value(const void* value)
{
    return text_reference(value);
}

const std::any& object_value(const void* value)
{
    if (!value)
    {
        throw std::runtime_error("deque 元素不能为 none");
    }
    return *static_cast<const std::any*>(value);
}

void object_result(const std::any& value, void** result)
{
    *result = make_handle<std::any>(value);
}

} // namespace

#define TX_DEQUE_SCALAR(SUFFIX, STORED, ABI, OUTPUT) \
extern "C" int txrt_deque_new_##SUFFIX(void** result) noexcept \
{ \
    return container_new<deque_storage<STORED>>(result); \
} \
extern "C" int txrt_deque_size_##SUFFIX(const void* value, std::int64_t* result) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        container_result(data.values.size(), result); \
    }); \
} \
extern "C" int txrt_deque_empty_##SUFFIX(const void* value, bool* result) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        *result = data.values.empty(); \
    }); \
} \
extern "C" int txrt_deque_clear_##SUFFIX(const void* value) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        data.values.clear(); \
    }); \
} \
extern "C" int txrt_deque_to_vector_##SUFFIX(const void* value, void** result) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        container_result(data.snapshot(), result); \
    }); \
} \
extern "C" int txrt_deque_push_front_##SUFFIX(const void* value, ABI item) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        data.values.push_front(stored_value(item)); \
    }); \
} \
extern "C" int txrt_deque_push_back_##SUFFIX(const void* value, ABI item) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        data.values.push_back(stored_value(item)); \
    }); \
} \
extern "C" int txrt_deque_pop_front_##SUFFIX(const void* value) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        data.pop_front(); \
    }); \
} \
extern "C" int txrt_deque_pop_back_##SUFFIX(const void* value) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        data.pop_back(); \
    }); \
} \
extern "C" int txrt_deque_front_##SUFFIX(const void* value, OUTPUT result) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        container_result(data.front(), result); \
    }); \
} \
extern "C" int txrt_deque_back_##SUFFIX(const void* value, OUTPUT result) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        container_result(data.back(), result); \
    }); \
} \
extern "C" int txrt_deque_read_##SUFFIX(const void* value, std::int64_t index, OUTPUT result) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        container_result(data.read(index), result); \
    }); \
} \
extern "C" int txrt_deque_set_##SUFFIX(const void* value, std::int64_t index, ABI item) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        data.set(index, stored_value(item)); \
    }); \
} \
extern "C" int txrt_deque_insert_##SUFFIX(const void* value, std::int64_t index, ABI item) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        data.insert(index, stored_value(item)); \
    }); \
} \
extern "C" int txrt_deque_erase_##SUFFIX(const void* value, std::int64_t index) noexcept \
{ \
    return container_apply<deque_storage<STORED>>(value, [&](auto& data) \
    { \
        data.erase(index); \
    }); \
}

TX_DEQUE_SCALAR(i64, std::int64_t, std::int64_t, std::int64_t*)
TX_DEQUE_SCALAR(f64, double, double, double*)
TX_DEQUE_SCALAR(bool, std::uint8_t, bool, bool*)
TX_DEQUE_SCALAR(str, text_reference, const void*, void**)

#undef TX_DEQUE_SCALAR

extern "C" int txrt_deque_new_object(const char* element_type, void** result) noexcept
{
    return invoke_checked([&]
    {
        if (!element_type)
        {
            throw std::runtime_error("deque 缺少元素类型");
        }
        auto storage = std::make_shared<object_deque_storage>(element_type);
        register_object_deque(storage);
        container_handle container = std::move(storage);
        *result = make_handle<std::any>(std::move(container));
    });
}

extern "C" int txrt_deque_size_object(const void* value, std::int64_t* result) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_deque_empty_object(const void* value, bool* result) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_deque_clear_object(const void* value) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_deque_to_vector_object(const void* value, void** result) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        container_result(data.snapshot(), result);
    });
}

extern "C" int txrt_deque_push_front_object(const void* value, const void* item) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        data.values.push_front(object_value(item));
    });
}

extern "C" int txrt_deque_push_back_object(const void* value, const void* item) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        data.values.push_back(object_value(item));
    });
}

extern "C" int txrt_deque_pop_front_object(const void* value) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        data.pop_front();
    });
}

extern "C" int txrt_deque_pop_back_object(const void* value) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        data.pop_back();
    });
}

extern "C" int txrt_deque_front_object(const void* value, void** result) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        object_result(data.front(), result);
    });
}

extern "C" int txrt_deque_back_object(const void* value, void** result) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        object_result(data.back(), result);
    });
}

extern "C" int txrt_deque_read_object(const void* value, std::int64_t index,
                                         void** result) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        object_result(data.read(index), result);
    });
}

extern "C" int txrt_deque_set_object(const void* value, std::int64_t index,
                                        const void* item) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        data.set(index, object_value(item));
    });
}

extern "C" int txrt_deque_insert_object(const void* value, std::int64_t index,
                                           const void* item) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        data.insert(index, object_value(item));
    });
}

extern "C" int txrt_deque_erase_object(const void* value, std::int64_t index) noexcept
{
    return container_apply<object_deque_storage>(value, [&](auto& data)
    {
        data.erase(index);
    });
}
