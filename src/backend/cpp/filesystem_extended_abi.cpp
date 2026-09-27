#include "backend/cpp/external_direct_abi.hpp"
#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/filesystem_extended.hpp"
#include "stdlib/filesystem_watch.hpp"
#include "stdlib/vector.hpp"

#include <any>
#include <cstdint>
#include <string>
#include <utility>

namespace
{

using tx_generated::detail::invoke_checked;
using tx_generated::detail::make_handle;

const std::string& text_value(const void* value)
{
    return *static_cast<const std::string*>(value);
}

const tx_generated::fs_watcher& watcher_value(const void* value)
{
    return std::any_cast<const tx_generated::fs_watcher&>(
        *static_cast<const std::any*>(value));
}

void* make_info(const char* type_name, tx_generated::file_info_value info)
{
    tx_generated::struct_fields fields(6);
    fields[0] = {"kind", std::move(info.kind)};
    fields[1] = {"size", info.size};
    fields[2] = {"permissions", info.permissions};
    fields[3] = {"created_millis", info.created_millis};
    fields[4] = {"accessed_millis", info.accessed_millis};
    fields[5] = {"modified_millis", info.modified_millis};
    return make_handle<std::any>(tx_generated::dynamic_struct(
        tx_generated::dynamic_struct_data{type_name, "file_info",
            std::move(fields)}));
}

void* make_event(const char* type_name, tx_generated::watch_event_value event)
{
    tx_generated::struct_fields fields(2);
    fields[0] = {"kind", std::move(event.kind)};
    fields[1] = {"path", std::move(event.path)};
    return make_handle<std::any>(tx_generated::dynamic_struct(
        tx_generated::dynamic_struct_data{type_name, "watch_event",
            std::move(fields)}));
}

void* make_string_vector(const std::vector<std::string>& names)
{
    tx_generated::string_vector result;
    auto& values = result.data().values;
    values.reserve(names.size());
    for (const auto& name : names)
    {
        auto* text = make_handle<std::string>(name);
        tx_generated::text_reference reference(text);
        tx_generated::detail::destroy_handle(text);
        values.push_back(std::move(reference));
    }
    result.data().refresh();
    return make_handle<std::any>(std::move(result));
}

} // namespace

extern "C" int txrt_fs_stat(const void* path, const char* type_name,
                             void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_info(type_name, tx_generated::fs_stat(text_value(path), true));
    }, tx::error_kind::io);
}

extern "C" int txrt_fs_lstat(const void* path, const char* type_name,
                              void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_info(type_name, tx_generated::fs_stat(text_value(path), false));
    }, tx::error_kind::io);
}

extern "C" int txrt_fs_set_permissions(const void* path,
                                        std::int64_t permissions) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::fs_set_permissions(text_value(path), permissions);
    }, tx::error_kind::io);
}

extern "C" int txrt_fs_create_symlink(const void* target,
    const void* link_path, bool directory) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::fs_create_symlink(text_value(target),
            text_value(link_path), directory);
    }, tx::error_kind::io);
}

extern "C" int txrt_fs_read_symlink(const void* path, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::string>(
            tx_generated::fs_read_symlink(text_value(path)));
    }, tx::error_kind::io);
}

extern "C" int txrt_fs_list_directory_filtered(const void* path,
    const void* kind, const void* extension, bool recursive,
    void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_string_vector(tx_generated::fs_list_directory_filtered(
            text_value(path), text_value(kind), text_value(extension), recursive));
    }, tx::error_kind::io);
}

extern "C" int txrt_fs_watch(const void* path, bool recursive,
                              void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_handle<std::any>(
            tx_generated::fs_watch(text_value(path), recursive));
    }, tx::error_kind::io);
}

extern "C" int txrt_fs_watch_next(const void* watcher,
    std::int64_t timeout_millis, const char* type_name, void** result) noexcept
{
    return invoke_checked([&]
    {
        *result = make_event(type_name,
            tx_generated::fs_watch_next(watcher_value(watcher), timeout_millis));
    }, tx::error_kind::io);
}

extern "C" int txrt_fs_close_watch(const void* watcher) noexcept
{
    return invoke_checked([&]
    {
        tx_generated::fs_close_watch(watcher_value(watcher));
    }, tx::error_kind::io);
}
