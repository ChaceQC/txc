#include "stdlib/random_generator.hpp"

#include "backend/cpp/vector_abi_internal.hpp"

#include <algorithm>
#include <any>
#include <cstdint>
#include <type_traits>

namespace tx_generated::random_instance
{
namespace
{

template<class element_type>
int shuffle(void* source, const void* input, void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        auto copy = detail::vector_value<element_type>(input).copy();
        auto& values = copy.data().values;
        for (std::size_t remaining = values.size(); remaining > 1; --remaining)
        {
            const auto chosen = next_index(source, remaining);
            std::swap(values[remaining - 1], values[chosen]);
        }
        copy.data().refresh();
        *result = detail::make_handle<std::any>(std::move(copy));
    });
}

template<class element_type>
int sample(void* source, const void* input, std::int64_t count,
           void** result) noexcept
{
    return detail::invoke_checked([&]
    {
        auto copy = detail::vector_value<element_type>(input).copy();
        auto& values = copy.data().values;
        if (count < 0 || static_cast<std::uint64_t>(count) > values.size())
        {
            fail("invalid_argument", "抽样数量超出向量范围");
        }
        for (std::size_t index = 0; index < static_cast<std::size_t>(count);
             ++index)
        {
            const auto chosen = index + next_index(source, values.size() - index);
            std::swap(values[index], values[chosen]);
        }
        values.resize(static_cast<std::size_t>(count));
        copy.data().refresh();
        *result = detail::make_handle<std::any>(std::move(copy));
    });
}

template<class element_type>
void write_choice(const element_type& value, void* result)
{
    if constexpr (std::is_same_v<element_type, text_reference>)
    {
        *static_cast<void**>(result) = detail::copy_text_handle(value.handle());
    }
    else if constexpr (std::is_same_v<element_type, byte_value> ||
                       std::is_same_v<element_type, std::any>)
    {
        *static_cast<void**>(result) = detail::make_handle<std::any>(value);
    }
    else if constexpr (std::is_same_v<element_type, std::uint8_t>)
    {
        *static_cast<bool*>(result) = value != 0;
    }
    else
    {
        *static_cast<element_type*>(result) = value;
    }
}

template<class element_type>
int choice(void* source, const void* input, void* result) noexcept
{
    return detail::invoke_checked([&]
    {
        const auto& values = detail::vector_value<element_type>(input).data().values;
        if (values.empty())
        {
            fail("invalid_argument", "不能从空向量选择元素");
        }
        write_choice(values[next_index(source, values.size())], result);
    });
}

} // namespace

#define TX_RANDOM_SAMPLING(SUFFIX, ELEMENT) \
extern "C" int txrt_random_shuffle_##SUFFIX(void* source, \
    const void* input, void** result) noexcept \
{ \
    return shuffle<ELEMENT>(source, input, result); \
} \
extern "C" int txrt_random_sample_##SUFFIX(void* source, \
    const void* input, std::int64_t count, void** result) noexcept \
{ \
    return sample<ELEMENT>(source, input, count, result); \
} \
extern "C" int txrt_random_choice_##SUFFIX(void* source, \
    const void* input, void* result) noexcept \
{ \
    return choice<ELEMENT>(source, input, result); \
}

TX_RANDOM_SAMPLING(i64, std::int64_t)
TX_RANDOM_SAMPLING(f64, double)
TX_RANDOM_SAMPLING(bool, std::uint8_t)
TX_RANDOM_SAMPLING(str, text_reference)
TX_RANDOM_SAMPLING(bytes, byte_value)
TX_RANDOM_SAMPLING(object, std::any)

#undef TX_RANDOM_SAMPLING

} // namespace tx_generated::random_instance
