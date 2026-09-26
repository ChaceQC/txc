#include "backend/cpp/vector_abi_internal.hpp"
#include "stdlib/algorithm_extended.hpp"

#include <any>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace
{

using namespace tx_generated;
using namespace tx_generated::detail;

const std::any& callback_of(const void* value)
{
    if (!value)
    {
        throw std::runtime_error("algorithm 缺少回调");
    }
    return *static_cast<const std::any*>(value);
}

template<class value_type>
[[nodiscard]] value_type native_value(const void* value)
{
    if constexpr (std::is_same_v<value_type, text_reference>)
    {
        return text_reference(value);
    }
    else if constexpr (std::is_same_v<value_type, std::any>)
    {
        return *static_cast<const std::any*>(value);
    }
    else if constexpr (std::is_same_v<value_type, byte_value>)
    {
        return std::any_cast<byte_value>(*static_cast<const std::any*>(value));
    }
    else if constexpr (std::is_same_v<value_type, std::uint8_t>)
    {
        return *static_cast<const bool*>(value);
    }
    else
    {
        return *static_cast<const value_type*>(value);
    }
}

template<class value_type>
void native_result(const value_type& value, void* result)
{
    if constexpr (std::is_same_v<value_type, text_reference>)
    {
        *static_cast<void**>(result) = retain_text_handle(value.handle());
    }
    else if constexpr (std::is_same_v<value_type, std::any> ||
                       std::is_same_v<value_type, byte_value>)
    {
        *static_cast<void**>(result) = make_handle<std::any>(value);
    }
    else if constexpr (std::is_same_v<value_type, std::uint8_t>)
    {
        *static_cast<bool*>(result) = value != 0;
    }
    else
    {
        *static_cast<value_type*>(result) = value;
    }
}

template<class element_type>
int stable_sort(const void* values, const void* compare,
    const void* less) noexcept
{
    return invoke_checked([&]
    {
        algorithm_stable_sort(vector_value<element_type>(values), compare, less);
    });
}

template<class element_type>
int stable_sorted(const void* values, const void* compare,
    const void* less, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(algorithm_stable_sorted(
            vector_value<element_type>(values), compare, less));
    });
}

template<class element_type>
int binary_search(const void* values, const void* key,
    const void* compare, const void* less, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = algorithm_binary_search(vector_value<element_type>(values),
            native_value<element_type>(key), compare, less);
    });
}

template<class element_type>
int equal_range(const void* values, const void* key,
    const void* compare, const void* less, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(algorithm_equal_range(
            vector_value<element_type>(values), native_value<element_type>(key),
            compare, less));
    });
}

template<class element_type>
int unique(const void* values, const void* compare,
    const void* less, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = algorithm_unique(vector_value<element_type>(values),
            compare, less);
    });
}

template<class element_type>
int rotate(const void* values, std::int64_t middle) noexcept
{
    return invoke_checked([&]
    {
        algorithm_rotate(vector_value<element_type>(values), middle);
    });
}

template<class element_type>
int partition(const void* values, const void* predicate,
    std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = algorithm_partition(vector_value<element_type>(values),
            callback_of(predicate));
    });
}

template<class input_type, class output_type>
int map(const void* values, const void* transform,
    const char* name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(algorithm_map<input_type, output_type>(
            vector_value<input_type>(values), callback_of(transform), name));
    });
}

template<class element_type>
int filter(const void* values, const void* predicate,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(algorithm_filter(
            vector_value<element_type>(values), callback_of(predicate)));
    });
}

template<class input_type, class output_type>
int fold(const void* values, const void* initial,
    const void* combine, void* result) noexcept
{
    return invoke_checked([&]
    {
        native_result(algorithm_fold<input_type, output_type>(
            vector_value<input_type>(values), native_value<output_type>(initial),
            callback_of(combine)), result);
    });
}

template<class element_type>
int all(const void* values, const void* predicate, bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = algorithm_all(vector_value<element_type>(values),
            callback_of(predicate));
    });
}

template<class element_type>
int any(const void* values, const void* predicate, bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = algorithm_any(vector_value<element_type>(values),
            callback_of(predicate));
    });
}

} // namespace

#define TX_ALGORITHM_EXT(SUFFIX, ELEMENT) \
extern "C" int txrt_algorithm_stable_sort_##SUFFIX(const void* values, \
    const void* compare, const void* less) noexcept \
{ \
    return stable_sort<ELEMENT>(values, compare, less); \
} \
extern "C" int txrt_algorithm_stable_sorted_##SUFFIX(const void* values, \
    const void* compare, const void* less, void** result) noexcept \
{ \
    return stable_sorted<ELEMENT>(values, compare, less, result); \
} \
extern "C" int txrt_algorithm_binary_search_##SUFFIX(const void* values, \
    const void* key, const void* compare, const void* less, \
    std::int64_t* result) noexcept \
{ \
    return binary_search<ELEMENT>(values, key, compare, less, result); \
} \
extern "C" int txrt_algorithm_equal_range_##SUFFIX(const void* values, \
    const void* key, const void* compare, const void* less, \
    void** result) noexcept \
{ \
    return equal_range<ELEMENT>(values, key, compare, less, result); \
} \
extern "C" int txrt_algorithm_unique_##SUFFIX(const void* values, \
    const void* compare, const void* less, std::int64_t* result) noexcept \
{ \
    return unique<ELEMENT>(values, compare, less, result); \
} \
extern "C" int txrt_algorithm_rotate_##SUFFIX(const void* values, \
    std::int64_t middle) noexcept \
{ \
    return rotate<ELEMENT>(values, middle); \
} \
extern "C" int txrt_algorithm_partition_##SUFFIX(const void* values, \
    const void* predicate, std::int64_t* result) noexcept \
{ \
    return partition<ELEMENT>(values, predicate, result); \
} \
extern "C" int txrt_algorithm_filter_##SUFFIX(const void* values, \
    const void* predicate, void** result) noexcept \
{ \
    return filter<ELEMENT>(values, predicate, result); \
} \
extern "C" int txrt_algorithm_all_##SUFFIX(const void* values, \
    const void* predicate, bool* result) noexcept \
{ \
    return all<ELEMENT>(values, predicate, result); \
} \
extern "C" int txrt_algorithm_any_##SUFFIX(const void* values, \
    const void* predicate, bool* result) noexcept \
{ \
    return any<ELEMENT>(values, predicate, result); \
}

TX_ALGORITHM_EXT(i64, std::int64_t)
TX_ALGORITHM_EXT(f64, double)
TX_ALGORITHM_EXT(bool, std::uint8_t)
TX_ALGORITHM_EXT(str, text_reference)
TX_ALGORITHM_EXT(bytes, byte_value)
TX_ALGORITHM_EXT(object, std::any)

#undef TX_ALGORITHM_EXT

#define TX_ALGORITHM_PAIR(INPUT_SUFFIX, OUTPUT_SUFFIX, INPUT, OUTPUT) \
extern "C" int txrt_algorithm_map_##INPUT_SUFFIX##_##OUTPUT_SUFFIX( \
    const void* values, const void* transform, const char* name, \
    void** result) noexcept \
{ \
    return map<INPUT, OUTPUT>(values, transform, name, result); \
} \
extern "C" int txrt_algorithm_fold_##INPUT_SUFFIX##_##OUTPUT_SUFFIX( \
    const void* values, const void* initial, const void* combine, \
    void* result) noexcept \
{ \
    return fold<INPUT, OUTPUT>(values, initial, combine, result); \
}

#define TX_ALGORITHM_OUTPUTS(INPUT_SUFFIX, INPUT) \
TX_ALGORITHM_PAIR(INPUT_SUFFIX, i64, INPUT, std::int64_t) \
TX_ALGORITHM_PAIR(INPUT_SUFFIX, f64, INPUT, double) \
TX_ALGORITHM_PAIR(INPUT_SUFFIX, bool, INPUT, std::uint8_t) \
TX_ALGORITHM_PAIR(INPUT_SUFFIX, str, INPUT, text_reference) \
TX_ALGORITHM_PAIR(INPUT_SUFFIX, bytes, INPUT, byte_value) \
TX_ALGORITHM_PAIR(INPUT_SUFFIX, object, INPUT, std::any)

TX_ALGORITHM_OUTPUTS(i64, std::int64_t)
TX_ALGORITHM_OUTPUTS(f64, double)
TX_ALGORITHM_OUTPUTS(bool, std::uint8_t)
TX_ALGORITHM_OUTPUTS(str, text_reference)
TX_ALGORITHM_OUTPUTS(bytes, byte_value)
TX_ALGORITHM_OUTPUTS(object, std::any)

#undef TX_ALGORITHM_OUTPUTS
#undef TX_ALGORITHM_PAIR
