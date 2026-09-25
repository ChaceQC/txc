#include "backend/cpp/external_direct_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "stdlib/stdlib.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <string>

namespace
{

const std::string& text_value(const void* value)
{
    return *static_cast<const std::string*>(value);
}

void* string_vector_value(const std::vector<std::string>& names)
{
    tx_generated::string_vector result;
    auto& values = result.data().values;
    values.reserve(names.size());
    for (const auto& name : names)
    {
        auto* text = tx_generated::detail::make_handle<std::string>(name);
        // 先转为内部文本引用，再移交给容器；不保留独立的根句柄。
        tx_generated::text_reference reference(text);
        tx_generated::detail::destroy_handle(text);
        values.push_back(std::move(reference));
    }
    result.data().refresh();
    return tx_generated::detail::make_handle<std::any>(std::move(result));
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

extern "C" int txrt_fs_list_directory_vector(const void* path, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = string_vector_value(tx_generated::tx_fn_list_directory_vector(
            text_value(path)));
    });
}

extern "C" int txrt_fs_walk_directory(const void* path, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = string_vector_value(tx_generated::tx_fn_walk_directory(text_value(path)));
    });
}

extern "C" int txrt_fs_copy_file(const void* source, const void* destination,
                                 bool overwrite) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_fn_copy_file(text_value(source), text_value(destination), overwrite);
    });
}

extern "C" int txrt_fs_rename(const void* source, const void* destination) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::tx_fn_rename(text_value(source), text_value(destination));
    });
}

extern "C" int txrt_fs_remove(const void* path, bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::tx_fn_remove(text_value(path));
    });
}

extern "C" int txrt_fs_remove_all(const void* path, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::tx_fn_remove_all(text_value(path));
    });
}

extern "C" int txrt_fs_file_size(const void* path, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::tx_fn_file_size(text_value(path));
    });
}

extern "C" int txrt_fs_modified_millis(const void* path, std::int64_t* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::tx_fn_modified_millis(text_value(path));
    });
}

extern "C" int txrt_path_normalize(const void* path, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::tx_fn_path_normalize(text_value(path)));
    });
}

extern "C" int txrt_path_is_absolute(const void* path, bool* result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::tx_fn_path_is_absolute(text_value(path));
    });
}

extern "C" int txrt_path_absolute(const void* path, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::tx_fn_path_absolute(text_value(path)));
    });
}

extern "C" int txrt_path_relative(const void* path, const void* base, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::tx_fn_path_relative(text_value(path), text_value(base)));
    });
}

extern "C" int txrt_path_replace_extension(const void* path, const void* extension,
                                          void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = tx_generated::detail::make_handle<std::string>(
            tx_generated::tx_fn_path_replace_extension(text_value(path), text_value(extension)));
    });
}
