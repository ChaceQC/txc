#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/sum_abi.hpp"
#include "backend/cpp/vector_abi_internal.hpp"
#include "stdlib/iterator.hpp"

#include <any>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace
{

using namespace tx_generated;
using namespace tx_generated::detail;

template<class element_type>
std::any box_element(const element_type& element)
{
    if constexpr (std::is_same_v<element_type, text_reference>)
    {
        return element.get();
    }
    else if constexpr (std::is_same_v<element_type, std::uint8_t>)
    {
        return element != 0;
    }
    else
    {
        return element;
    }
}

template<class element_type>
int create_iterator(const void* source, const char* element_name,
                    bool snapshot, void** result) noexcept
{
    return invoke_checked([&]
    {
        auto& vector = vector_value<element_type>(source);
        std::any values = snapshot ? std::any(vector.copy()) : std::any(vector);
        *result = make_handle<std::any>(tx_iterator(iterator_state{
            std::move(values), element_name}));
    });
}

template<class element_type>
const element_type* read_next(const void* source)
{
    auto& iterator = std::any_cast<const tx_iterator&>(
        *static_cast<const std::any*>(source)).data();
    if (iterator.closed)
    {
        throw runtime_failure({tx::error_kind::runtime,
            "invalid_state", "已关闭的 iterator 不能继续读取"});
    }
    if (iterator.exhausted)
    {
        return nullptr;
    }
    const auto& values = std::any_cast<const tx_vector<element_type>&>(
        iterator.values).data().values;
    if (iterator.index < values.size())
    {
        return &values[iterator.index++];
    }
    iterator.exhausted = true;
    return nullptr;
}

template<class element_type>
int next_element(const void* source, const char* option_type,
                 void** result) noexcept
{
    return invoke_checked([&]
    {
        const auto* item = read_next<element_type>(source);
        int status = 0;
        if constexpr (std::is_same_v<element_type, std::int64_t>)
        {
            status = txrt_option_new_i64(option_type, item != nullptr,
                item ? *item : 0, result);
        }
        else if constexpr (std::is_same_v<element_type, double>)
        {
            status = txrt_option_new_f64(option_type, item != nullptr,
                item ? *item : 0.0, result);
        }
        else if constexpr (std::is_same_v<element_type, std::uint8_t>)
        {
            status = txrt_option_new_bool(option_type, item != nullptr,
                item && *item != 0, result);
        }
        else
        {
            std::any boxed = item ? box_element(*item) : std::any{};
            status = txrt_option_new(option_type, item != nullptr,
                item ? &boxed : nullptr, result);
        }
        if (status != 0)
        {
            throw runtime_failure({current_runtime_context().last_error_kind,
                current_runtime_context().last_error_code, current_runtime_context().last_error});
        }
    });
}

template<class element_type, class scalar_type>
int next_scalar(const void* source, bool* present,
                scalar_type* value) noexcept
{
    return invoke_checked([&]
    {
        const auto* item = read_next<element_type>(source);
        *present = item != nullptr;
        *value = item ? static_cast<scalar_type>(*item) : scalar_type{};
    });
}

} // namespace

#define TX_ITERATOR(SUFFIX, ELEMENT) \
extern "C" int txrt_iterator_new_##SUFFIX(const void* source, \
    const char* element_name, bool snapshot, void** result) noexcept \
{ \
    return create_iterator<ELEMENT>(source, element_name, snapshot, result); \
} \
extern "C" int txrt_iterator_next_##SUFFIX(const void* source, \
    const char* option_type, void** result) noexcept \
{ \
    return next_element<ELEMENT>(source, option_type, result); \
}

TX_ITERATOR(i64, std::int64_t)
TX_ITERATOR(f64, double)
TX_ITERATOR(bool, std::uint8_t)
TX_ITERATOR(str, text_reference)
TX_ITERATOR(bytes, byte_value)
TX_ITERATOR(object, std::any)

#undef TX_ITERATOR

extern "C" int txrt_iterator_next_scalar_i64(const void* source,
    bool* present, std::int64_t* value) noexcept
{
    return next_scalar<std::int64_t>(source, present, value);
}

extern "C" int txrt_iterator_next_scalar_f64(const void* source,
    bool* present, double* value) noexcept
{
    return next_scalar<double>(source, present, value);
}

extern "C" int txrt_iterator_next_scalar_bool(const void* source,
    bool* present, bool* value) noexcept
{
    return next_scalar<std::uint8_t>(source, present, value);
}

extern "C" int txrt_iterator_close(const void* source) noexcept
{
    return invoke_checked([&]
    {
        auto& iterator = std::any_cast<const tx_iterator&>(
            *static_cast<const std::any*>(source)).data();
        iterator.closed = true;
        iterator.values.reset();
    });
}
