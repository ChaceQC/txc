#include "backend/cpp/value_abi.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <any>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace
{

template<class value_type>
value_type argument(const void* const* arguments, std::size_t index)
{
    return std::any_cast<const value_type&>(
        *static_cast<const std::any*>(arguments[index]));
}

std::size_t expected_arity(std::string_view name)
{
    if (name == "flush")
    {
        return 0;
    }
    if (name == "write" || name == "write_line" ||
        name == "write_error" || name == "trim" ||
        name == "lower" || name == "upper")
    {
        return 1;
    }
    if (name == "contains" || name == "starts_with" ||
        name == "ends_with" || name == "find" ||
        name == "split" || name == "join" || name == "read_text")
    {
        return 2;
    }
    if (name == "slice" || name == "replace" ||
        name == "write_text" || name == "append_text")
    {
        return 3;
    }
    throw std::runtime_error("未知标准库函数：" + std::string(name));
}

std::any dispatch(std::string_view name, const void* const* arguments,
                  std::size_t count)
{
    if (count != expected_arity(name))
    {
        throw std::runtime_error("标准库函数参数数量不匹配：" + std::string(name));
    }
    using tx_generated::tx_array;
    using tx_generated::tx_int;
    if (name == "write")
    {
        tx_generated::tx_fn_write(argument<std::string>(arguments, 0));
    }
    else if (name == "write_line")
    {
        tx_generated::tx_fn_write_line(argument<std::string>(arguments, 0));
    }
    else if (name == "write_error")
    {
        tx_generated::tx_fn_write_error(argument<std::string>(arguments, 0));
    }
    else if (name == "flush")
    {
        tx_generated::tx_fn_flush();
    }
    else if (name == "contains")
    {
        return tx_generated::tx_fn_contains(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "starts_with")
    {
        return tx_generated::tx_fn_starts_with(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "ends_with")
    {
        return tx_generated::tx_fn_ends_with(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "find")
    {
        return tx_generated::tx_fn_find(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "slice")
    {
        return tx_generated::tx_fn_slice(
            argument<std::string>(arguments, 0),
            argument<tx_int>(arguments, 1),
            argument<tx_int>(arguments, 2));
    }
    else if (name == "replace")
    {
        return tx_generated::tx_fn_replace(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1),
            argument<std::string>(arguments, 2));
    }
    else if (name == "split")
    {
        return tx_generated::tx_fn_split(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "join")
    {
        return tx_generated::tx_fn_join(
            argument<tx_array>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "trim")
    {
        return tx_generated::tx_fn_trim(argument<std::string>(arguments, 0));
    }
    else if (name == "lower")
    {
        return tx_generated::tx_fn_lower(argument<std::string>(arguments, 0));
    }
    else if (name == "upper")
    {
        return tx_generated::tx_fn_upper(argument<std::string>(arguments, 0));
    }
    else if (name == "read_text")
    {
        return tx_generated::tx_fn_read_text(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "write_text")
    {
        tx_generated::tx_fn_write_text(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1),
            argument<std::string>(arguments, 2));
    }
    else if (name == "append_text")
    {
        tx_generated::tx_fn_append_text(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1),
            argument<std::string>(arguments, 2));
    }
    else
    {
        throw std::runtime_error("未知标准库函数：" + std::string(name));
    }
    return {};
}

} // namespace

extern "C" int txrt_call_external(const char* name,
                                    const void* const* arguments,
                                    std::size_t count,
                                    void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&] {
        *result = new std::any(dispatch(name, arguments, count));
    });
}
