#include "backend/cpp/vector_abi.hpp"
#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/vector_abi_internal.hpp"

#include <string_view>

namespace
{

tx_generated::string_vector split_vector(std::string_view source,
                                         std::string_view delimiter)
{
    if (delimiter.empty())
    {
        throw std::runtime_error("split_vector 的分隔符不能为空");
    }
    tx_generated::string_vector vector;
    auto& values = vector.data().values;
    std::size_t count = 1;
    std::size_t offset = 0;
    while ((offset = source.find(delimiter, offset)) != std::string_view::npos)
    {
        ++count;
        offset += delimiter.size();
    }
    values.reserve(count);
    std::size_t start = 0;
    while (true)
    {
        const auto end = source.find(delimiter, start);
        auto* handle = tx_generated::detail::make_handle<std::string>(source.substr(start,
            end == std::string_view::npos ? end : end - start));
        values.emplace_back(handle);
        tx_generated::detail::destroy_handle(handle);
        if (end == std::string_view::npos)
        {
            break;
        }
        start = end + delimiter.size();
    }
    vector.data().refresh();
    return vector;
}

std::string join_vector(const void* vector, std::string_view delimiter)
{
    const auto& values = tx_generated::detail::vector_value<
        tx_generated::text_reference>(vector).data().values;
    std::string joined;
    std::size_t total = 0;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        const auto length = values[index].get().size();
        const auto extra = index == 0 ? 0 : delimiter.size();
        if (length > joined.max_size() - total ||
            extra > joined.max_size() - total - length)
        {
            throw std::length_error("join 结果字符串过长");
        }
        total += length + extra;
    }
    joined.reserve(total);
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index != 0)
        {
            joined += delimiter;
        }
        joined += values[index].get();
    }
    return joined;
}

} // namespace

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

extern "C" void txrt_vector_index_error() noexcept
{
    std::snprintf(tx_generated::detail::last_error, 256, "vector 索引越界");
    txrt_require_success(1);
}

extern "C" void txrt_vector_store_str(void* slot, const void* item) noexcept
{
    // 先增加新值引用，再释放旧值，支持同一元素的自赋值。
    *static_cast<tx_generated::text_reference*>(slot) =
        tx_generated::text_reference(item);
}

extern "C" int txrt_string_split_vector_literal(
    const void* text, const char* separator, std::size_t length, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(split_vector(
            *static_cast<const std::string*>(text), std::string_view(separator, length)));
    });
}

extern "C" int txrt_string_join_vector_literal(
    const void* vector, const char* separator, std::size_t length, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(join_vector(
            vector, std::string_view(separator, length)));
    });
}

extern "C" int txrt_string_split_vector(
    const void* text, const void* separator, void** result) noexcept
{
    const auto& delimiter = *static_cast<const std::string*>(separator);
    return txrt_string_split_vector_literal(text, delimiter.data(), delimiter.size(), result);
}

extern "C" int txrt_string_join_vector(
    const void* vector, const void* separator, void** result) noexcept
{
    const auto& delimiter = *static_cast<const std::string*>(separator);
    return txrt_string_join_vector_literal(vector, delimiter.data(), delimiter.size(), result);
}
