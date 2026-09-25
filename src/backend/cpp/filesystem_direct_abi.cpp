#include "backend/cpp/external_direct_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"

#include <any>
#include <string>

namespace
{

const std::string& text_value(const void* value)
{
    return *static_cast<const std::string*>(value);
}

} // namespace

using tx_generated::detail::invoke_checked;

extern "C" int txrt_fs_exists(const void* path, bool* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_fn_exists(text_value(path));
    });
}

extern "C" int txrt_fs_is_file(const void* path, bool* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_fn_is_file(text_value(path));
    });
}

extern "C" int txrt_fs_is_directory(const void* path, bool* result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::tx_fn_is_directory(text_value(path));
    });
}

extern "C" int txrt_fs_create_directories(const void* path) noexcept
{
    return invoke_checked([&] {
        tx_generated::tx_fn_create_directories(text_value(path));
    });
}

extern "C" int txrt_fs_list_directory(const void* path,
                                        void** result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::detail::make_handle<std::any>(tx_generated::tx_fn_list_directory(
            text_value(path)));
    });
}

extern "C" int txrt_path_join(const void* left, const void* right,
                                void** result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::detail::make_handle<std::string>(tx_generated::tx_fn_path_join(
            text_value(left), text_value(right)));
    });
}

extern "C" int txrt_path_parent(const void* path, void** result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::tx_fn_parent(text_value(path)));
    });
}

extern "C" int txrt_path_file_name(const void* path,
                                     void** result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::tx_fn_file_name(text_value(path)));
    });
}

extern "C" int txrt_path_extension(const void* path,
                                     void** result) noexcept
{
    return invoke_checked([&] {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::tx_fn_extension(text_value(path)));
    });
}
