#pragma once

#include <cstdint>

extern "C"
{

int txrt_db_postgres_options(const void* host, const void* database, const void* user,
    const void* ca_file, const char* type_name, void** result) noexcept;
int txrt_db_postgres_version(std::int64_t* result) noexcept;
int txrt_db_open_postgres(const void* config, const void* password, void** result) noexcept;
int txrt_db_sqlstate(const void* connection, void** result) noexcept;
int txrt_db_transaction_status(const void* connection, void** result) noexcept;
int txrt_db_pool(const void* config, std::int64_t capacity, std::int64_t waiters, void** result) noexcept;
int txrt_db_postgres_pool(const void* config, const void* password,
    std::int64_t capacity, std::int64_t waiters, void** result) noexcept;
int txrt_db_acquire(const void* pool, std::int64_t timeout_ms, void** result) noexcept;
int txrt_db_release(const void* connection, bool* result) noexcept;
int txrt_db_close_pool(const void* pool, bool* result) noexcept;

int txrt_db_sqlite_options(const void* path, const char* type_name, void** result) noexcept;
int txrt_db_sqlite_version(void** result) noexcept;
int txrt_db_open(const void* config, void** result) noexcept;
int txrt_db_close(const void* connection, bool* result) noexcept;
int txrt_db_extended_error_code(const void* connection, std::int64_t* result) noexcept;
int txrt_db_prepare(const void* connection, const void* sql, void** result) noexcept;
int txrt_db_parameter_count(const void* statement, std::int64_t* result) noexcept;
int txrt_db_parameter_index(const void* statement, const void* name, std::int64_t* result) noexcept;
int txrt_db_bind(const void* statement, std::int64_t index, const void* value) noexcept;
int txrt_db_clear_bindings(const void* statement) noexcept;
int txrt_db_query(const void* statement, void** result) noexcept;
int txrt_db_close_statement(const void* statement, bool* result) noexcept;
int txrt_db_close_cursor(const void* cursor, bool* result) noexcept;
int txrt_db_begin(const void* connection, const void* mode, void** result) noexcept;
int txrt_db_commit(const void* transaction) noexcept;
int txrt_db_rollback(const void* transaction, bool* result) noexcept;
int txrt_db_savepoint(const void* transaction, const void* name) noexcept;
int txrt_db_rollback_to(const void* transaction, const void* name) noexcept;
int txrt_db_release_savepoint(const void* transaction, const void* name) noexcept;
int txrt_db_bind_all(const void* statement, const void* params) noexcept;
int txrt_db_execute(const void* statement, const char* type_name, void** result) noexcept;
int txrt_db_next(const void* cursor, const char* type_name, void** result) noexcept;
int txrt_db_column_count(const void* row, std::int64_t* result) noexcept;
int txrt_db_column_name(const void* row, std::int64_t index, void** result) noexcept;
int txrt_db_column_index(const void* row, const void* name, std::int64_t* result) noexcept;
int txrt_db_get_value(const void* row, std::int64_t index, void** result) noexcept;
int txrt_db_null_value(void** result) noexcept;
int txrt_db_int_value(std::int64_t value, void** result) noexcept;
int txrt_db_float_value(double value, void** result) noexcept;
int txrt_db_bool_value(bool value, void** result) noexcept;
int txrt_db_str_value(const void* value, void** result) noexcept;
int txrt_db_bytes_value(const void* value, void** result) noexcept;
int txrt_db_value_kind(const void* value, void** result) noexcept;
int txrt_db_decimal_value(const void* value, void** result) noexcept;
int txrt_db_datetime_value(const void* value, void** result) noexcept;
int txrt_db_as_int(const void* value, const char* type_name, void** result) noexcept;
int txrt_db_get_int(const void* row, std::int64_t index, const char* type_name, void** result) noexcept;
int txrt_db_as_float(const void* value, const char* type_name, void** result) noexcept;
int txrt_db_get_float(const void* row, std::int64_t index, const char* type_name, void** result) noexcept;
int txrt_db_as_bool(const void* value, const char* type_name, void** result) noexcept;
int txrt_db_get_bool(const void* row, std::int64_t index, const char* type_name, void** result) noexcept;
int txrt_db_as_str(const void* value, const char* type_name, void** result) noexcept;
int txrt_db_get_str(const void* row, std::int64_t index, const char* type_name, void** result) noexcept;
int txrt_db_as_bytes(const void* value, const char* type_name, void** result) noexcept;
int txrt_db_get_bytes(const void* row, std::int64_t index, const char* type_name, void** result) noexcept;
int txrt_db_as_decimal(const void* value, const char* type_name, const char* element_type, void** result) noexcept;
int txrt_db_get_decimal(const void* row, std::int64_t index, const char* type_name, const char* element_type, void** result) noexcept;
int txrt_db_as_datetime(const void* value, const char* type_name, const char* element_type, void** result) noexcept;
int txrt_db_get_datetime(const void* row, std::int64_t index, const char* type_name, const char* element_type, void** result) noexcept;

}
