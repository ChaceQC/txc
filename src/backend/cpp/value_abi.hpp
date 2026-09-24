#pragma once

#include <cstddef>
#include <cstdint>

// array、dict、any 和结构体在 LLVM 后端中使用独立的 std::any 句柄。
// 返回的句柄由调用方持有；address 函数返回借用的内部字段地址。
extern "C"
{

int txrt_value_none(void** result) noexcept;
int txrt_value_box_i64(std::int64_t value, void** result) noexcept;
int txrt_value_box_f64(double value, void** result) noexcept;
int txrt_value_box_bool(bool value, void** result) noexcept;
int txrt_value_box_str(const void* value, void** result) noexcept;
int txrt_value_clone(const void* value, void** result) noexcept;
void txrt_value_release(void* value) noexcept;
int txrt_value_assign(void* target, const void* value) noexcept;
int txrt_value_to_i64(const void* value, std::int64_t* result) noexcept;
int txrt_value_to_f64(const void* value, double* result) noexcept;
int txrt_value_to_bool(const void* value, bool* result) noexcept;
int txrt_value_to_str(const void* value, void** result) noexcept;
int txrt_value_is_none(const void* value, bool* result) noexcept;
int txrt_value_len(const void* value, std::int64_t* result) noexcept;
int txrt_value_print(const void* value) noexcept;
int txrt_value_require_type(const void* value, const char* type_name) noexcept;

int txrt_array_new(std::int64_t length, void** result) noexcept;
int txrt_array_resize(std::int64_t length, const void* initial,
                       void** result) noexcept;
int txrt_array_len(const void* value, std::int64_t* result) noexcept;
int txrt_array_element_address(void* value, std::int64_t index,
                               void** result) noexcept;
int txrt_array_append(void* value, const void* item) noexcept;
int txrt_array_extend(void* value, const void* items) noexcept;
int txrt_array_require_length(const void* value, std::size_t length) noexcept;

int txrt_dict_new(void** result) noexcept;
int txrt_dict_set(void* value, const void* key, const void* item) noexcept;
int txrt_dict_len(const void* value, std::int64_t* result) noexcept;
int txrt_dict_key_address(void* value, std::int64_t index,
                          void** result) noexcept;
int txrt_value_element_address(void* value, const void* key, bool create,
                                void** result) noexcept;

int txrt_struct_new(const char* type_name, std::size_t field_count,
                    void** result) noexcept;
int txrt_struct_set_field(void* value, std::size_t index,
                          const char* field_name, const void* field) noexcept;
int txrt_struct_field_address(void* value, const char* field_name,
                              void** result) noexcept;
int txrt_call_external(const char* name, const void* const* arguments,
                       std::size_t count, void** result) noexcept;

}
