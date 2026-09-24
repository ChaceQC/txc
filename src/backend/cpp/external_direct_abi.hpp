#pragma once

#include <cstdint>

// 外部模块目标和类型已在编译期确定，句柄参数保持现有值语义。
extern "C"
{

int txrt_string_contains(const void* text, const void* part, bool* result) noexcept;
int txrt_string_starts_with(const void* text, const void* prefix,
                            bool* result) noexcept;
int txrt_string_ends_with(const void* text, const void* suffix,
                          bool* result) noexcept;
int txrt_string_find(const void* text, const void* part,
                      std::int64_t* result) noexcept;
int txrt_string_slice(const void* text, std::int64_t start,
                       std::int64_t end, void** result) noexcept;
int txrt_string_replace(const void* text, const void* old,
                         const void* replacement, void** result) noexcept;
int txrt_string_split(const void* text, const void* separator,
                       void** result) noexcept;
int txrt_string_join(const void* parts, const void* separator,
                      void** result) noexcept;
int txrt_string_trim(const void* text, void** result) noexcept;
int txrt_string_lower(const void* text, void** result) noexcept;
int txrt_string_upper(const void* text, void** result) noexcept;

int txrt_array_concat(const void* left, const void* right,
                       void** result) noexcept;
int txrt_array_slice(const void* values, std::int64_t start,
                      std::int64_t end, void** result) noexcept;
int txrt_array_reverse(const void* values, void** result) noexcept;

int txrt_file_read_text(const void* path, const void* encoding,
                         void** result) noexcept;
int txrt_file_write_text(const void* path, const void* text,
                          const void* encoding) noexcept;
int txrt_file_append_text(const void* path, const void* text,
                           const void* encoding) noexcept;

int txrt_fs_exists(const void* path, bool* result) noexcept;
int txrt_fs_is_file(const void* path, bool* result) noexcept;
int txrt_fs_is_directory(const void* path, bool* result) noexcept;
int txrt_fs_create_directories(const void* path) noexcept;
int txrt_fs_list_directory(const void* path, void** result) noexcept;
int txrt_path_join(const void* left, const void* right,
                    void** result) noexcept;
int txrt_path_parent(const void* path, void** result) noexcept;
int txrt_path_file_name(const void* path, void** result) noexcept;
int txrt_path_extension(const void* path, void** result) noexcept;

int txrt_io_write(const void* text) noexcept;
int txrt_io_write_line(const void* text) noexcept;
int txrt_io_write_error(const void* text) noexcept;
int txrt_io_flush() noexcept;

int txrt_time_unix_millis(std::int64_t* result) noexcept;
int txrt_time_monotonic_millis(std::int64_t* result) noexcept;
int txrt_time_sleep_millis(std::int64_t duration) noexcept;

}
