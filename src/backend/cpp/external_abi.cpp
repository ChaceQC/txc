#include "backend/cpp/value_abi.hpp"

#include "backend/cpp/external_abi_time_random.hpp"
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
    if (name == "io.flush" || name == "time.unix_millis" ||
        name == "time.monotonic_millis" || name == "random.random_float")
    {
        return 0;
    }
    if (name == "io.write" || name == "io.write_line" ||
        name == "io.write_error" || name == "string.trim" ||
        name == "string.lower" || name == "string.upper" ||
        name == "math.abs" || name == "math.sqrt" ||
        name == "math.floor" || name == "math.ceil" ||
        name == "array.reverse" || name == "fs.exists" ||
        name == "fs.is_file" || name == "fs.is_directory" ||
        name == "fs.create_directories" || name == "fs.list_directory" ||
        name == "path.parent" || name == "path.file_name" ||
        name == "path.extension" || name == "time.sleep_millis" ||
        name == "random.seed")
    {
        return 1;
    }
    if (name == "string.contains" || name == "string.starts_with" ||
        name == "string.ends_with" || name == "string.find" ||
        name == "string.split" || name == "string.join" ||
        name == "file.read_text" || name == "math.min" ||
        name == "math.max" || name == "math.mod" ||
        name == "math.pow" || name == "array.concat" ||
        name == "path.join" || name == "random.random_int")
    {
        return 2;
    }
    if (name == "string.slice" || name == "string.replace" ||
        name == "file.write_text" || name == "file.append_text" ||
        name == "math.clamp" || name == "array.slice")
    {
        return 3;
    }
    throw std::runtime_error("未知标准库函数：" + std::string(name));
}

std::any dispatch_math(std::string_view name, const void* const* arguments)
{
    using tx_generated::tx_int;
    const bool integers = static_cast<const std::any*>(arguments[0])->type() ==
                          typeid(tx_int);
    if (name == "math.abs")
    {
        return integers
            ? std::any(tx_generated::tx_fn_abs(argument<tx_int>(arguments, 0)))
            : std::any(tx_generated::tx_fn_abs(argument<double>(arguments, 0)));
    }
    if (name == "math.min")
    {
        return integers
            ? std::any(tx_generated::tx_fn_min(argument<tx_int>(arguments, 0),
                                               argument<tx_int>(arguments, 1)))
            : std::any(tx_generated::tx_fn_min(argument<double>(arguments, 0),
                                               argument<double>(arguments, 1)));
    }
    if (name == "math.max")
    {
        return integers
            ? std::any(tx_generated::tx_fn_max(argument<tx_int>(arguments, 0),
                                               argument<tx_int>(arguments, 1)))
            : std::any(tx_generated::tx_fn_max(argument<double>(arguments, 0),
                                               argument<double>(arguments, 1)));
    }
    if (name == "math.clamp")
    {
        return integers
            ? std::any(tx_generated::tx_fn_clamp(
                  argument<tx_int>(arguments, 0),
                  argument<tx_int>(arguments, 1),
                  argument<tx_int>(arguments, 2)))
            : std::any(tx_generated::tx_fn_clamp(
                  argument<double>(arguments, 0),
                  argument<double>(arguments, 1),
                  argument<double>(arguments, 2)));
    }
    if (name == "math.mod")
    {
        return tx_generated::tx_fn_mod(argument<tx_int>(arguments, 0),
                                       argument<tx_int>(arguments, 1));
    }
    if (name == "math.sqrt")
    {
        return tx_generated::tx_fn_sqrt(argument<double>(arguments, 0));
    }
    if (name == "math.pow")
    {
        return tx_generated::tx_fn_pow(argument<double>(arguments, 0),
                                       argument<double>(arguments, 1));
    }
    if (name == "math.floor")
    {
        return tx_generated::tx_fn_floor(argument<double>(arguments, 0));
    }
    if (name == "math.ceil")
    {
        return tx_generated::tx_fn_ceil(argument<double>(arguments, 0));
    }
    throw std::runtime_error("未知数学函数：" + std::string(name));
}

std::any dispatch_array(std::string_view name, const void* const* arguments)
{
    using tx_generated::tx_array;
    using tx_generated::tx_int;
    if (name == "array.concat")
    {
        return tx_generated::tx_fn_concat(
            argument<tx_array>(arguments, 0),
            argument<tx_array>(arguments, 1));
    }
    if (name == "array.slice")
    {
        return tx_generated::tx_fn_array_slice(
            argument<tx_array>(arguments, 0),
            argument<tx_int>(arguments, 1),
            argument<tx_int>(arguments, 2));
    }
    if (name == "array.reverse")
    {
        return tx_generated::tx_fn_reverse(argument<tx_array>(arguments, 0));
    }
    throw std::runtime_error("未知数组函数：" + std::string(name));
}

std::any dispatch_filesystem(std::string_view name,
                             const void* const* arguments)
{
    if (name == "fs.exists")
    {
        return tx_generated::tx_fn_exists(argument<std::string>(arguments, 0));
    }
    if (name == "fs.is_file")
    {
        return tx_generated::tx_fn_is_file(argument<std::string>(arguments, 0));
    }
    if (name == "fs.is_directory")
    {
        return tx_generated::tx_fn_is_directory(
            argument<std::string>(arguments, 0));
    }
    if (name == "fs.create_directories")
    {
        tx_generated::tx_fn_create_directories(
            argument<std::string>(arguments, 0));
        return {};
    }
    if (name == "fs.list_directory")
    {
        return tx_generated::tx_fn_list_directory(
            argument<std::string>(arguments, 0));
    }
    if (name == "path.join")
    {
        return tx_generated::tx_fn_path_join(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    if (name == "path.parent")
    {
        return tx_generated::tx_fn_parent(argument<std::string>(arguments, 0));
    }
    if (name == "path.file_name")
    {
        return tx_generated::tx_fn_file_name(
            argument<std::string>(arguments, 0));
    }
    if (name == "path.extension")
    {
        return tx_generated::tx_fn_extension(
            argument<std::string>(arguments, 0));
    }
    throw std::runtime_error("未知文件系统函数：" + std::string(name));
}

std::any dispatch(std::string_view name, const void* const* arguments,
                  std::size_t count)
{
    if (count != expected_arity(name))
    {
        throw std::runtime_error("标准库函数参数数量不匹配：" + std::string(name));
    }
    if (name.starts_with("time.") || name.starts_with("random."))
    {
        return tx_generated::detail::dispatch_time_random(name, arguments);
    }
    if (name.starts_with("math."))
    {
        return dispatch_math(name, arguments);
    }
    if (name.starts_with("array."))
    {
        return dispatch_array(name, arguments);
    }
    if (name.starts_with("fs.") || name.starts_with("path."))
    {
        return dispatch_filesystem(name, arguments);
    }
    using tx_generated::tx_array;
    using tx_generated::tx_int;
    if (name == "io.write")
    {
        tx_generated::tx_fn_write(argument<std::string>(arguments, 0));
    }
    else if (name == "io.write_line")
    {
        tx_generated::tx_fn_write_line(argument<std::string>(arguments, 0));
    }
    else if (name == "io.write_error")
    {
        tx_generated::tx_fn_write_error(argument<std::string>(arguments, 0));
    }
    else if (name == "io.flush")
    {
        tx_generated::tx_fn_flush();
    }
    else if (name == "string.contains")
    {
        return tx_generated::tx_fn_contains(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "string.starts_with")
    {
        return tx_generated::tx_fn_starts_with(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "string.ends_with")
    {
        return tx_generated::tx_fn_ends_with(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "string.find")
    {
        return tx_generated::tx_fn_find(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "string.slice")
    {
        return tx_generated::tx_fn_slice(
            argument<std::string>(arguments, 0),
            argument<tx_int>(arguments, 1),
            argument<tx_int>(arguments, 2));
    }
    else if (name == "string.replace")
    {
        return tx_generated::tx_fn_replace(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1),
            argument<std::string>(arguments, 2));
    }
    else if (name == "string.split")
    {
        return tx_generated::tx_fn_split(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "string.join")
    {
        return tx_generated::tx_fn_join(
            argument<tx_array>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "string.trim")
    {
        return tx_generated::tx_fn_trim(argument<std::string>(arguments, 0));
    }
    else if (name == "string.lower")
    {
        return tx_generated::tx_fn_lower(argument<std::string>(arguments, 0));
    }
    else if (name == "string.upper")
    {
        return tx_generated::tx_fn_upper(argument<std::string>(arguments, 0));
    }
    else if (name == "file.read_text")
    {
        return tx_generated::tx_fn_read_text(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1));
    }
    else if (name == "file.write_text")
    {
        tx_generated::tx_fn_write_text(
            argument<std::string>(arguments, 0),
            argument<std::string>(arguments, 1),
            argument<std::string>(arguments, 2));
    }
    else if (name == "file.append_text")
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
