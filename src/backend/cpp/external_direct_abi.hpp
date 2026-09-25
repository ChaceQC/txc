#pragma once

#include <cstddef>
#include <cstdint>

// 外部模块目标和类型已在编译期确定，句柄参数保持现有值语义。
extern "C"
{
int txrt_dictionary_get_concat(const void* values, const void* first,
                              const void* second, void** result) noexcept;
int txrt_dictionary_contains_concat(const void* values, const void* first,
                                   const void* second, bool* result) noexcept;

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
int txrt_string_join_literal(const void* parts, const char* separator,
                            std::size_t length, void** result) noexcept;
int txrt_string_contains_literal(const void* text, const char* part,
                                  std::size_t length, bool* result) noexcept;
int txrt_string_starts_with_literal(const void* text, const char* prefix,
                                     std::size_t length, bool* result) noexcept;
int txrt_string_ends_with_literal(const void* text, const char* suffix,
                                   std::size_t length, bool* result) noexcept;
int txrt_string_find_literal(const void* text, const char* part,
                              std::size_t length, std::int64_t* result) noexcept;
int txrt_string_split_literal(const void* text, const char* separator,
                               std::size_t length, void** result) noexcept;
int txrt_string_replace_literal(const void* text, const char* old,
                                 std::size_t old_length, const char* replacement,
                                 std::size_t replacement_length,
                                 void** result) noexcept;
int txrt_string_trim(const void* text, void** result) noexcept;
int txrt_string_lower(const void* text, void** result) noexcept;
int txrt_string_upper(const void* text, void** result) noexcept;
int txrt_format_format(const void* text, const void* args,
                       const void* kwargs, void** result) noexcept;

int txrt_array_concat(const void* left, const void* right,
                       void** result) noexcept;
int txrt_array_slice(const void* values, std::int64_t start,
                      std::int64_t end, void** result) noexcept;
int txrt_array_reverse(const void* values, void** result) noexcept;
int txrt_array_push_back(void* values, const void* value) noexcept;
int txrt_array_pop_back(void* values) noexcept;
int txrt_array_insert(void* values, std::int64_t index, const void* value) noexcept;
int txrt_array_erase(void* values, std::int64_t index) noexcept;
int txrt_array_clear(void* values) noexcept;

int txrt_dictionary_get(const void* values, const void* key,
                         void** result) noexcept;
int txrt_dictionary_get_str(const void* values, const void* key,
                             void** result) noexcept;
int txrt_dictionary_contains(const void* values, const void* key,
                              bool* result) noexcept;
int txrt_dictionary_contains_str(const void* values, const void* key,
                                  bool* result) noexcept;
int txrt_dictionary_remove(void* values, const void* key,
                            bool* result) noexcept;
int txrt_dictionary_remove_str(void* values, const void* key,
                                bool* result) noexcept;
int txrt_dictionary_keys(const void* values, void** result) noexcept;
int txrt_dictionary_values(const void* values, void** result) noexcept;
int txrt_dictionary_items(const void* values, void** result) noexcept;
int txrt_dictionary_clear(void* values) noexcept;

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
int txrt_fs_list_directory_vector(const void* path, void** result) noexcept;
int txrt_fs_walk_directory(const void* path, void** result) noexcept;
int txrt_fs_copy_file(const void* source, const void* destination, bool overwrite) noexcept;
int txrt_fs_rename(const void* source, const void* destination) noexcept;
int txrt_fs_remove(const void* path, bool* result) noexcept;
int txrt_fs_remove_all(const void* path, std::int64_t* result) noexcept;
int txrt_fs_file_size(const void* path, std::int64_t* result) noexcept;
int txrt_fs_modified_millis(const void* path, std::int64_t* result) noexcept;
int txrt_path_join(const void* left, const void* right,
                    void** result) noexcept;
int txrt_path_parent(const void* path, void** result) noexcept;
int txrt_path_file_name(const void* path, void** result) noexcept;
int txrt_path_extension(const void* path, void** result) noexcept;
int txrt_path_normalize(const void* path, void** result) noexcept;
int txrt_path_is_absolute(const void* path, bool* result) noexcept;
int txrt_path_absolute(const void* path, void** result) noexcept;
int txrt_path_relative(const void* path, const void* base, void** result) noexcept;
int txrt_path_replace_extension(const void* path, const void* extension,
                                 void** result) noexcept;

int txrt_system_initialize() noexcept;
int txrt_system_args(void** result) noexcept;
int txrt_system_current_directory(void** result) noexcept;
int txrt_system_set_current_directory(const void* path) noexcept;
int txrt_system_executable_path(void** result) noexcept;
int txrt_system_temp_directory(void** result) noexcept;
int txrt_system_home_directory(void** result) noexcept;
int txrt_env_contains(const void* name, bool* result) noexcept;
int txrt_env_get(const void* name, void** result) noexcept;
int txrt_env_get_default(const void* name, const void* default_value,
                          void** result) noexcept;
int txrt_env_set(const void* name, const void* value) noexcept;
int txrt_env_remove(const void* name, bool* result) noexcept;

int txrt_io_write(const void* text) noexcept;
int txrt_io_write_line(const void* text) noexcept;
int txrt_io_write_error(const void* text) noexcept;
int txrt_io_flush() noexcept;

int txrt_time_unix_millis(std::int64_t* result) noexcept;
int txrt_time_monotonic_millis(std::int64_t* result) noexcept;
int txrt_time_monotonic_micros(std::int64_t* result) noexcept;
int txrt_time_sleep_millis(std::int64_t duration) noexcept;

}
