#include "backend/cpp/typed_container_abi.hpp"
#include "backend/cpp/container_abi_internal.hpp"
#include "stdlib/typed_sequence.hpp"

using namespace tx_generated;
using namespace tx_generated::detail;

// 这些单步操作已由语义分析选择具体元素类型。让 ThinLTO 将其展开到
// 调用循环，合并重复的容器解包；异常转换和空队列检查仍保留。
#ifdef __clang__
#define TX_QUEUE_STEP __attribute__((always_inline))
#else
#define TX_QUEUE_STEP
#endif

extern "C" int txrt_queue_new_i64(void** result) noexcept
{
    return container_new<queue_storage<std::int64_t>>(result);
}

extern "C" int txrt_queue_size_i64(const void* value, std::int64_t* result) noexcept
{
    return container_apply<queue_storage<std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_queue_empty_i64(const void* value, bool* result) noexcept
{
    return container_apply<queue_storage<std::int64_t>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_queue_clear_i64(const void* value) noexcept
{
    return container_apply<queue_storage<std::int64_t>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_queue_to_vector_i64(const void* value, void** result) noexcept
{
    return container_apply<queue_storage<std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.snapshot(), result);
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_push_i64(const void* value, std::int64_t item) noexcept
{
    return container_apply<queue_storage<std::int64_t>>(value, [&](auto& data)
    {
        data.push(item);
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_pop_i64(const void* value) noexcept
{
    return container_apply<queue_storage<std::int64_t>>(value, [&](auto& data)
    {
        data.pop();
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_front_i64(const void* value, std::int64_t* result) noexcept
{
    return container_apply<queue_storage<std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.front(), result);
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_back_i64(const void* value, std::int64_t* result) noexcept
{
    return container_apply<queue_storage<std::int64_t>>(value, [&](auto& data)
    {
        container_result(data.back(), result);
    });
}

extern "C" int txrt_queue_new_f64(void** result) noexcept
{
    return container_new<queue_storage<double>>(result);
}

extern "C" int txrt_queue_size_f64(const void* value, std::int64_t* result) noexcept
{
    return container_apply<queue_storage<double>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_queue_empty_f64(const void* value, bool* result) noexcept
{
    return container_apply<queue_storage<double>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_queue_clear_f64(const void* value) noexcept
{
    return container_apply<queue_storage<double>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_queue_to_vector_f64(const void* value, void** result) noexcept
{
    return container_apply<queue_storage<double>>(value, [&](auto& data)
    {
        container_result(data.snapshot(), result);
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_push_f64(const void* value, double item) noexcept
{
    return container_apply<queue_storage<double>>(value, [&](auto& data)
    {
        data.push(item);
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_pop_f64(const void* value) noexcept
{
    return container_apply<queue_storage<double>>(value, [&](auto& data)
    {
        data.pop();
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_front_f64(const void* value, double* result) noexcept
{
    return container_apply<queue_storage<double>>(value, [&](auto& data)
    {
        container_result(data.front(), result);
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_back_f64(const void* value, double* result) noexcept
{
    return container_apply<queue_storage<double>>(value, [&](auto& data)
    {
        container_result(data.back(), result);
    });
}

extern "C" int txrt_queue_new_bool(void** result) noexcept
{
    return container_new<queue_storage<std::uint8_t>>(result);
}

extern "C" int txrt_queue_size_bool(const void* value, std::int64_t* result) noexcept
{
    return container_apply<queue_storage<std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_queue_empty_bool(const void* value, bool* result) noexcept
{
    return container_apply<queue_storage<std::uint8_t>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_queue_clear_bool(const void* value) noexcept
{
    return container_apply<queue_storage<std::uint8_t>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_queue_to_vector_bool(const void* value, void** result) noexcept
{
    return container_apply<queue_storage<std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.snapshot(), result);
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_push_bool(const void* value, bool item) noexcept
{
    return container_apply<queue_storage<std::uint8_t>>(value, [&](auto& data)
    {
        data.push(item);
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_pop_bool(const void* value) noexcept
{
    return container_apply<queue_storage<std::uint8_t>>(value, [&](auto& data)
    {
        data.pop();
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_front_bool(const void* value, bool* result) noexcept
{
    return container_apply<queue_storage<std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.front(), result);
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_back_bool(const void* value, bool* result) noexcept
{
    return container_apply<queue_storage<std::uint8_t>>(value, [&](auto& data)
    {
        container_result(data.back(), result);
    });
}

extern "C" int txrt_queue_new_str(void** result) noexcept
{
    return container_new<queue_storage<text_reference>>(result);
}

extern "C" int txrt_queue_size_str(const void* value, std::int64_t* result) noexcept
{
    return container_apply<queue_storage<text_reference>>(value, [&](auto& data)
    {
        container_result(data.values.size(), result);
    });
}

extern "C" int txrt_queue_empty_str(const void* value, bool* result) noexcept
{
    return container_apply<queue_storage<text_reference>>(value, [&](auto& data)
    {
        *result = data.values.empty();
    });
}

extern "C" int txrt_queue_clear_str(const void* value) noexcept
{
    return container_apply<queue_storage<text_reference>>(value, [&](auto& data)
    {
        data.values.clear();
    });
}

extern "C" int txrt_queue_to_vector_str(const void* value, void** result) noexcept
{
    return container_apply<queue_storage<text_reference>>(value, [&](auto& data)
    {
        container_result(data.snapshot(), result);
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_push_str(const void* value, const void* item) noexcept
{
    return container_apply<queue_storage<text_reference>>(value, [&](auto& data)
    {
        data.push(text_reference(item));
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_pop_str(const void* value) noexcept
{
    return container_apply<queue_storage<text_reference>>(value, [&](auto& data)
    {
        data.pop();
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_front_str(const void* value, void** result) noexcept
{
    return container_apply<queue_storage<text_reference>>(value, [&](auto& data)
    {
        container_result(data.front(), result);
    });
}

extern "C" TX_QUEUE_STEP int txrt_queue_back_str(const void* value, void** result) noexcept
{
    return container_apply<queue_storage<text_reference>>(value, [&](auto& data)
    {
        container_result(data.back(), result);
    });
}

#undef TX_QUEUE_STEP
