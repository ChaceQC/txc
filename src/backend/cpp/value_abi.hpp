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
bool txrt_value_snapshot_scalar(const void* value, std::uint8_t* kind,
                                 std::uint64_t* bits) noexcept;
int txrt_value_snapshot_box(std::uint8_t kind, std::uint64_t bits,
                             const void* fallback, void** result) noexcept;
std::int64_t txrt_value_snapshot_to_i64_fast(std::uint8_t kind,
                                              std::uint64_t bits,
                                              const void* fallback) noexcept;
double txrt_value_snapshot_to_f64_fast(std::uint8_t kind,
                                        std::uint64_t bits,
                                        const void* fallback) noexcept;
bool txrt_value_snapshot_to_bool_fast(std::uint8_t kind,
                                       std::uint64_t bits,
                                       const void* fallback) noexcept;
int txrt_value_deep_copy(const void* value, void** result) noexcept;
int txrt_value_deep_copy_known(const void* value, std::uint64_t kind, void** result) noexcept;

#define TX_DICT_SCALAR_KEY_DECL(SUFFIX, TYPE) \
std::int64_t txrt_dict_ref_get_i64_key_##SUFFIX(const void* value, TYPE key) noexcept; \
double txrt_dict_ref_get_f64_key_##SUFFIX(const void* value, TYPE key) noexcept; \
void* txrt_dict_ref_get_str_key_##SUFFIX(const void* value, TYPE key) noexcept; \
bool txrt_dict_ref_contains_##SUFFIX(const void* value, TYPE key) noexcept;

TX_DICT_SCALAR_KEY_DECL(i64, std::int64_t)
TX_DICT_SCALAR_KEY_DECL(f64, double)
TX_DICT_SCALAR_KEY_DECL(bool, bool)

#undef TX_DICT_SCALAR_KEY_DECL
void txrt_value_release(void* value) noexcept;
int txrt_value_assign(void* target, const void* value) noexcept;
int txrt_value_set_i64(void* target, std::int64_t value) noexcept;
int txrt_value_set_f64(void* target, double value) noexcept;
int txrt_value_set_bool(void* target, bool value) noexcept;
int txrt_value_to_i64(const void* value, std::int64_t* result) noexcept;
int txrt_value_to_f64(const void* value, double* result) noexcept;
int txrt_value_to_bool(const void* value, bool* result) noexcept;
std::int64_t txrt_value_to_i64_fast(const void* value) noexcept;
double txrt_value_to_f64_fast(const void* value) noexcept;
bool txrt_value_to_bool_fast(const void* value) noexcept;
int txrt_value_to_str(const void* value, void** result) noexcept;
int txrt_value_is_none(const void* value, bool* result) noexcept;
int txrt_value_len(const void* value, std::int64_t* result) noexcept;
int txrt_value_print(const void* value, bool newline) noexcept;
int txrt_value_require_type(const void* value, const char* type_name) noexcept;
int txrt_value_require_type_or_none(const void* value,
                                    const char* type_name) noexcept;

int txrt_array_new(std::int64_t length, void** result) noexcept;
void* txrt_array_ref(void* value) noexcept;
std::int64_t txrt_array_ref_len(const void* value) noexcept;
std::int64_t txrt_array_ref_get_i64(const void* value,
                                    std::int64_t index) noexcept;
double txrt_array_ref_get_f64(const void* value,
                              std::int64_t index) noexcept;
void* txrt_array_ref_get_str(const void* value,
                             std::int64_t index) noexcept;
const void* txrt_array_ref_element_read_ptr(const void* value,
                                            std::int64_t index) noexcept;
void* txrt_array_ref_element_ptr(void* value, std::int64_t index) noexcept;
int txrt_array_ref_set_i64(void* value, std::int64_t index,
                           std::int64_t item) noexcept;
int txrt_array_ref_set_f64(void* value, std::int64_t index,
                           double item) noexcept;
int txrt_array_ref_set_bool(void* value, std::int64_t index,
                            bool item) noexcept;
int txrt_array_ref_set_str(void* value, std::int64_t index,
                           const void* item) noexcept;
void txrt_array_index_error() noexcept;
int txrt_local_scalar_array_new(std::int64_t length, void** result) noexcept;
void txrt_local_scalar_array_release(void* value) noexcept;
int txrt_array_resize(std::int64_t length, const void* initial,
                       void** result) noexcept;
int txrt_array_len(const void* value, std::int64_t* result) noexcept;
int txrt_array_element_address(void* value, std::int64_t index,
                               void** result) noexcept;
void* txrt_array_element_ptr(void* value, std::int64_t index) noexcept;
const void* txrt_array_element_read_ptr(const void* value,
                                        std::int64_t index) noexcept;
int txrt_array_set_i64(void* value, std::int64_t index,
                       std::int64_t item) noexcept;
int txrt_array_set_f64(void* value, std::int64_t index,
                       double item) noexcept;
int txrt_array_set_bool(void* value, std::int64_t index,
                        bool item) noexcept;
int txrt_array_set_str(void* value, std::int64_t index,
                       const void* item) noexcept;
int txrt_array_set_value(void* value, std::int64_t index,
                         const void* item) noexcept;
int txrt_array_append(void* value, const void* item) noexcept;
int txrt_array_extend(void* value, const void* items) noexcept;
int txrt_array_require_length(const void* value, std::size_t length) noexcept;

int txrt_dict_new(void** result) noexcept;
void* txrt_dict_ref(void* value) noexcept;
std::int64_t txrt_dict_ref_len(const void* value) noexcept;
const void* txrt_dict_ref_element_read_ptr(const void* value,
                                           const void* key) noexcept;
const void* txrt_dict_ref_element_read_ptr_str(const void* value,
                                               const void* key) noexcept;
const void* txrt_dict_ref_element_read_ptr_literal(const void* value,
    const char* key, std::size_t length) noexcept;
std::int64_t txrt_dict_ref_get_i64(const void* value,
                                  const void* key) noexcept;
std::int64_t txrt_dict_ref_get_i64_str(const void* value,
                                      const void* key) noexcept;
std::int64_t txrt_dict_ref_get_i64_literal(const void* value,
    const char* key, std::size_t length) noexcept;
double txrt_dict_ref_get_f64(const void* value,
                             const void* key) noexcept;
double txrt_dict_ref_get_f64_str(const void* value,
                                 const void* key) noexcept;
double txrt_dict_ref_get_f64_literal(const void* value,
    const char* key, std::size_t length) noexcept;
void* txrt_dict_ref_get_str(const void* value,
                            const void* key) noexcept;
void* txrt_dict_ref_get_str_str(const void* value,
                                const void* key) noexcept;
void* txrt_dict_ref_get_str_literal(const void* value,
    const char* key, std::size_t length) noexcept;
int txrt_dict_set(void* value, const void* key, const void* item) noexcept;
int txrt_dict_set_i64_str(void* value, const void* key,
                          std::int64_t item) noexcept;
int txrt_dict_set_f64_str(void* value, const void* key,
                          double item) noexcept;
int txrt_dict_set_bool_str(void* value, const void* key,
                           bool item) noexcept;
int txrt_dict_set_str_str(void* value, const void* key,
                          const void* item) noexcept;
int txrt_dict_len(const void* value, std::int64_t* result) noexcept;
int txrt_dict_element_address(void* value, const void* key, bool create,
                               void** result) noexcept;
int txrt_dict_element_address_str(void* value, const void* key, bool create,
                                   void** result) noexcept;
int txrt_dict_element_address_literal(void* value, const char* key,
                                       std::size_t length, bool create,
                                       void** result) noexcept;
int txrt_value_element_address(void* value, const void* key, bool create,
                               void** result) noexcept;

int txrt_struct_new(const char* type_name, const char* display_name,
                    std::size_t field_count, void** result) noexcept;
int txrt_struct_set_field(void* value, std::size_t index,
                          const char* field_name, const void* field) noexcept;
int txrt_struct_set_field_i64(void* value, std::size_t index,
                              const char* field_name,
                              std::int64_t field) noexcept;
int txrt_struct_set_field_f64(void* value, std::size_t index,
                              const char* field_name, double field) noexcept;
int txrt_struct_set_field_bool(void* value, std::size_t index,
                               const char* field_name, bool field) noexcept;
int txrt_struct_field_address(void* value, const char* field_name,
                               void** result) noexcept;
int txrt_struct_field_address_index(void* value, std::size_t index,
                                     void** result) noexcept;
void* txrt_struct_field_i64_ptr(const void* value,
                                std::size_t index) noexcept;
void* txrt_struct_field_f64_ptr(const void* value,
                                std::size_t index) noexcept;
void* txrt_struct_field_bool_ptr(const void* value,
                                 std::size_t index) noexcept;
int txrt_class_new(const char* type_name, const char* display_name,
                   const char* const* ancestors, std::size_t ancestor_count,
                   const char* const* field_types, std::size_t field_count,
                   const void* const* virtual_targets, std::size_t virtual_count,
                   const void* const* destructor_targets,
                   std::size_t destructor_count,
                   void** result) noexcept;
int txrt_class_field_address_index(void* value, std::size_t index,
                                    bool for_write, void** result) noexcept;
void* txrt_class_field_i64_ptr(const void* value, std::size_t index) noexcept;
void* txrt_class_field_f64_ptr(const void* value, std::size_t index) noexcept;
void* txrt_class_field_bool_ptr(const void* value, std::size_t index) noexcept;
int txrt_class_virtual_target(const void* value, std::size_t slot,
                               void** result) noexcept;
void* txrt_class_virtual_target_fast(const void* value,
                                      std::size_t slot) noexcept;
int txrt_class_require_type(const void* value,
                            const char* type_name) noexcept;
void txrt_class_require_type_fast(const void* value,
                                  const char* type_name) noexcept;
int txrt_call_external(const char* name, const void* const* arguments,
                       std::size_t count, void** result) noexcept;

}
