#include "backend/cpp/container_abi_internal.hpp"
#include "stdlib/typed_sequence.hpp"

#include <any>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace
{

using namespace tx_generated;
using namespace tx_generated::detail;

const std::any& object_value(const void* value)
{
    if (!value)
    {
        throw std::runtime_error("容器元素不能为 none");
    }
    return *static_cast<const std::any*>(value);
}

template<class element_type>
int make_heap(bool descending, const char* name, const void* less,
              const void* compare, const void* input, void** result) noexcept
{
    return invoke_checked([&]
    {
        if (!name)
        {
            throw std::runtime_error("heap 缺少元素类型");
        }
        const auto callback = compare ? object_value(compare) : std::any{};
        auto storage = std::make_shared<heap_storage<element_type>>(
            descending, name, less, callback);
        register_heap_storage(storage);
        if (input)
        {
            const auto& vector = std::any_cast<const tx_vector<element_type>&>(
                object_value(input));
            storage->assign(vector);
        }
        container_handle container = std::move(storage);
        *result = make_handle<std::any>(std::move(container));
    });
}

template<class element_type>
int make_queue(const char* name, const void* input, void** result) noexcept
{
    return invoke_checked([&]
    {
        if (!name)
        {
            throw std::runtime_error("queue 缺少元素类型");
        }
        auto storage = std::make_shared<queue_storage<element_type>>(name);
        register_queue_storage(storage);
        if (input)
        {
            const auto& vector = std::any_cast<const tx_vector<element_type>&>(
                object_value(input));
            storage->assign(vector);
        }
        container_handle container = std::move(storage);
        *result = make_handle<std::any>(std::move(container));
    });
}

} // namespace

#define TX_HEAP_BUILD(SUFFIX, ELEMENT) \
extern "C" int txrt_heap_build_##SUFFIX(bool descending, const char* name, \
    const void* less, const void* compare, const void* input, void** result) noexcept \
{ \
    return make_heap<ELEMENT>(descending, name, less, compare, input, result); \
}

TX_HEAP_BUILD(i64, std::int64_t)
TX_HEAP_BUILD(f64, double)
TX_HEAP_BUILD(bool, std::uint8_t)
TX_HEAP_BUILD(str, text_reference)
TX_HEAP_BUILD(object, std::any)

#undef TX_HEAP_BUILD

#define TX_QUEUE_BUILD(SUFFIX, ELEMENT) \
extern "C" int txrt_queue_build_##SUFFIX(const char* name, \
    const void* input, void** result) noexcept \
{ \
    return make_queue<ELEMENT>(name, input, result); \
}

TX_QUEUE_BUILD(i64, std::int64_t)
TX_QUEUE_BUILD(f64, double)
TX_QUEUE_BUILD(bool, std::uint8_t)
TX_QUEUE_BUILD(str, text_reference)
TX_QUEUE_BUILD(object, std::any)

#undef TX_QUEUE_BUILD

extern "C" int txrt_heap_new_object(bool descending, const char* name,
    const void* less, void** result) noexcept
{
    return make_heap<std::any>(descending, name, less, nullptr, nullptr, result);
}

extern "C" int txrt_queue_new_object(const char* name, void** result) noexcept
{
    return make_queue<std::any>(name, nullptr, result);
}

extern "C" int txrt_heap_size_object(const void* value, std::int64_t* result) noexcept
{
    return container_apply<heap_storage<std::any>>(value, [&](auto& data)
    {
        data.require_ready();
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_heap_empty_object(const void* value, bool* result) noexcept
{
    return container_apply<heap_storage<std::any>>(value, [&](auto& data)
    {
        data.require_ready();
        *result = data.values.empty();
    });
}

extern "C" int txrt_heap_clear_object(const void* value) noexcept
{
    return container_apply<heap_storage<std::any>>(value, [&](auto& data)
    {
        data.require_ready();
        data.values.clear();
    });
}

extern "C" int txrt_heap_to_vector_object(const void* value, void** result) noexcept
{
    return container_apply<heap_storage<std::any>>(value, [&](auto& data)
    {
        container_result(data.snapshot(), result);
    });
}

extern "C" int txrt_heap_push_object(const void* value, const void* item) noexcept
{
    return container_apply<heap_storage<std::any>>(value, [&](auto& data)
    {
        data.push(object_value(item));
    });
}

extern "C" int txrt_heap_pop_object(const void* value) noexcept
{
    return container_apply<heap_storage<std::any>>(value, [&](auto& data)
    {
        data.pop();
    });
}

extern "C" int txrt_heap_top_object(const void* value, void** result) noexcept
{
    return container_apply<heap_storage<std::any>>(value, [&](auto& data)
    {
        *result = make_handle<std::any>(data.top());
    });
}

extern "C" int txrt_queue_size_object(const void* value, std::int64_t* result) noexcept
{
    return container_apply<queue_storage<std::any>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_queue_empty_object(const void* value, bool* result) noexcept
{
    return container_apply<queue_storage<std::any>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_queue_clear_object(const void* value) noexcept
{
    return container_apply<queue_storage<std::any>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_queue_to_vector_object(const void* value, void** result) noexcept
{
    return container_apply<queue_storage<std::any>>(value, [&](auto& data)
    {
        container_result(data.snapshot(), result);
    });
}

extern "C" int txrt_queue_push_object(const void* value, const void* item) noexcept
{
    return container_apply<queue_storage<std::any>>(value, [&](auto& data)
    {
        data.push(object_value(item));
    });
}

extern "C" int txrt_queue_pop_object(const void* value) noexcept
{
    return container_apply<queue_storage<std::any>>(value, [&](auto& data)
    {
        data.pop();
    });
}

extern "C" int txrt_queue_front_object(const void* value, void** result) noexcept
{
    return container_apply<queue_storage<std::any>>(value, [&](auto& data)
    {
        *result = make_handle<std::any>(data.front());
    });
}

extern "C" int txrt_queue_back_object(const void* value, void** result) noexcept
{
    return container_apply<queue_storage<std::any>>(value, [&](auto& data)
    {
        *result = make_handle<std::any>(data.back());
    });
}
