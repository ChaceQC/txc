#include "backend/cpp/external_direct_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/dictionary.hpp"

#include <array>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace
{

const std::any* find_joined_key(const void* dictionary,
                              const void* first, const void* second)
{
    const auto& values = std::any_cast<const tx_generated::tx_dict&>(
        *static_cast<const std::any*>(dictionary));
    const auto& left = *static_cast<const std::string*>(first);
    const auto& right = *static_cast<const std::string*>(second);
    // 短复合键只需临时字节，按原哈希函数查询，不创建 TX 字符串句柄。
    std::array<char, 256> local;
    if (left.size() <= local.size() && right.size() <= local.size() - left.size())
    {
        std::memcpy(local.data(), left.data(), left.size());
        std::memcpy(local.data() + left.size(), right.data(), right.size());
        return values.find_value(std::string_view(local.data(), left.size() + right.size()));
    }
    std::string joined;
    if (right.size() > joined.max_size() - left.size())
    {
        throw std::length_error("字符串拼接结果过长");
    }
    joined.reserve(left.size() + right.size());
    joined.append(left);
    joined.append(right);
    return values.find_value(std::string_view(joined));
}

} // namespace

extern "C" int txrt_dictionary_get_concat(const void* values,
    const void* first, const void* second, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        const auto* found = find_joined_key(values, first, second);
        *result = tx_generated::detail::make_handle<std::any>(found ? *found : std::any{});
    });
}

extern "C" int txrt_dictionary_contains_concat(const void* values,
    const void* first, const void* second, bool* result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        *result = find_joined_key(values, first, second) != nullptr;
    });
}
