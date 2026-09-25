#pragma once

#include <cstdint>
#include <cstddef>

extern "C"
{

void txrt_vector_index_error() noexcept;
void txrt_vector_store_str(void* slot, const void* item) noexcept;
int txrt_string_split_vector(const void* text, const void* separator, void** result) noexcept;
int txrt_string_join_vector(const void* values, const void* separator, void** result) noexcept;
int txrt_string_split_vector_literal(const void* text, const char* separator,
                                    std::size_t length, void** result) noexcept;
int txrt_string_join_vector_literal(const void* values, const char* separator,
                                   std::size_t length, void** result) noexcept;
void* txrt_vector_ref_i64(const void* value) noexcept;
int txrt_vector_new_i64(std::int64_t count, std::int64_t item, void** result) noexcept;
int txrt_vector_from_array_i64(const void* value, void** result) noexcept;
int txrt_vector_to_array_i64(const void* value, void** result) noexcept;
int txrt_vector_reserve_i64(void* value, std::int64_t count) noexcept;
int txrt_vector_push_back_i64(void* value, std::int64_t item) noexcept;
int txrt_vector_resize_i64(void* value, std::int64_t count, std::int64_t item) noexcept;
int txrt_vector_clear_i64(void* value) noexcept;
int txrt_vector_pop_back_i64(void* value) noexcept;
int txrt_vector_insert_i64(void* value, std::int64_t index, std::int64_t item) noexcept;
int txrt_vector_erase_i64(void* value, std::int64_t index) noexcept;
void* txrt_vector_ref_f64(const void* value) noexcept;
int txrt_vector_new_f64(std::int64_t count, double item, void** result) noexcept;
int txrt_vector_from_array_f64(const void* value, void** result) noexcept;
int txrt_vector_to_array_f64(const void* value, void** result) noexcept;
int txrt_vector_reserve_f64(void* value, std::int64_t count) noexcept;
int txrt_vector_push_back_f64(void* value, double item) noexcept;
int txrt_vector_resize_f64(void* value, std::int64_t count, double item) noexcept;
int txrt_vector_clear_f64(void* value) noexcept;
int txrt_vector_pop_back_f64(void* value) noexcept;
int txrt_vector_insert_f64(void* value, std::int64_t index, double item) noexcept;
int txrt_vector_erase_f64(void* value, std::int64_t index) noexcept;
void* txrt_vector_ref_bool(const void* value) noexcept;
int txrt_vector_new_bool(std::int64_t count, bool item, void** result) noexcept;
int txrt_vector_from_array_bool(const void* value, void** result) noexcept;
int txrt_vector_to_array_bool(const void* value, void** result) noexcept;
int txrt_vector_reserve_bool(void* value, std::int64_t count) noexcept;
int txrt_vector_push_back_bool(void* value, bool item) noexcept;
int txrt_vector_resize_bool(void* value, std::int64_t count, bool item) noexcept;
int txrt_vector_clear_bool(void* value) noexcept;
int txrt_vector_pop_back_bool(void* value) noexcept;
int txrt_vector_insert_bool(void* value, std::int64_t index, bool item) noexcept;
int txrt_vector_erase_bool(void* value, std::int64_t index) noexcept;
void* txrt_vector_ref_str(const void* value) noexcept;
int txrt_vector_new_str(std::int64_t count, const void* item, void** result) noexcept;
int txrt_vector_from_array_str(const void* value, void** result) noexcept;
int txrt_vector_to_array_str(const void* value, void** result) noexcept;
int txrt_vector_reserve_str(void* value, std::int64_t count) noexcept;
int txrt_vector_push_back_str(void* value, const void* item) noexcept;
int txrt_vector_resize_str(void* value, std::int64_t count, const void* item) noexcept;
int txrt_vector_clear_str(void* value) noexcept;
int txrt_vector_pop_back_str(void* value) noexcept;
int txrt_vector_insert_str(void* value, std::int64_t index, const void* item) noexcept;
int txrt_vector_erase_str(void* value, std::int64_t index) noexcept;
void* txrt_vector_ref_bytes(const void* value) noexcept;
int txrt_vector_new_bytes(std::int64_t count, const void* item, void** result) noexcept;
int txrt_vector_from_array_bytes(const void* value, void** result) noexcept;
int txrt_vector_to_array_bytes(const void* value, void** result) noexcept;
int txrt_vector_reserve_bytes(void* value, std::int64_t count) noexcept;
int txrt_vector_push_back_bytes(void* value, const void* item) noexcept;
int txrt_vector_resize_bytes(void* value, std::int64_t count, const void* item) noexcept;
int txrt_vector_clear_bytes(void* value) noexcept;
int txrt_vector_pop_back_bytes(void* value) noexcept;
int txrt_vector_insert_bytes(void* value, std::int64_t index, const void* item) noexcept;
int txrt_vector_erase_bytes(void* value, std::int64_t index) noexcept;
int txrt_vector_get_bytes(const void* value, std::int64_t index, void** result) noexcept;
int txrt_vector_set_bytes(void* value, std::int64_t index, const void* item) noexcept;

}
