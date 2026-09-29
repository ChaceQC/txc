#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::write_db_declarations()
{
    module_ << "declare i32 @txrt_db_postgres_options(ptr, ptr, ptr, ptr, ptr, ptr)\n"
            << "declare i32 @txrt_db_postgres_version(ptr)\n"
            << "declare i32 @txrt_db_open_postgres(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_db_sqlstate(ptr, ptr)\n"
            << "declare i32 @txrt_db_transaction_status(ptr, ptr)\n"
            << "declare i32 @txrt_db_pool(ptr, i64, i64, ptr)\n"
            << "declare i32 @txrt_db_postgres_pool(ptr, ptr, i64, i64, ptr)\n"
            << "declare i32 @txrt_db_acquire(ptr, i64, ptr)\n"
            << "declare i32 @txrt_db_release(ptr, ptr)\n"
            << "declare i32 @txrt_db_close_pool(ptr, ptr)\n"
            << "declare i32 @txrt_db_sqlite_options(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_db_sqlite_version(ptr)\n"
            << "declare i32 @txrt_db_open(ptr, ptr)\n"
            << "declare i32 @txrt_db_close(ptr, ptr)\n"
            << "declare i32 @txrt_db_extended_error_code(ptr, ptr)\n"
            << "declare i32 @txrt_db_prepare(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_db_parameter_count(ptr, ptr)\n"
            << "declare i32 @txrt_db_parameter_index(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_db_bind(ptr, i64, ptr)\n"
            << "declare i32 @txrt_db_bind_all(ptr, ptr)\n"
            << "declare i32 @txrt_db_clear_bindings(ptr)\n"
            << "declare i32 @txrt_db_execute(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_db_query(ptr, ptr)\n"
            << "declare i32 @txrt_db_close_statement(ptr, ptr)\n"
            << "declare i32 @txrt_db_next(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_db_close_cursor(ptr, ptr)\n"
            << "declare i32 @txrt_db_column_count(ptr, ptr)\n"
            << "declare i32 @txrt_db_column_name(ptr, i64, ptr)\n"
            << "declare i32 @txrt_db_column_index(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_db_get_value(ptr, i64, ptr)\n"
            << "declare i32 @txrt_db_null_value(ptr)\n"
            << "declare i32 @txrt_db_int_value(i64, ptr)\n"
            << "declare i32 @txrt_db_float_value(double, ptr)\n"
            << "declare i32 @txrt_db_bool_value(i1, ptr)\n"
            << "declare i32 @txrt_db_str_value(ptr, ptr)\n"
            << "declare i32 @txrt_db_bytes_value(ptr, ptr)\n"
            << "declare i32 @txrt_db_decimal_value(ptr, ptr)\n"
            << "declare i32 @txrt_db_datetime_value(ptr, ptr)\n"
            << "declare i32 @txrt_db_value_kind(ptr, ptr)\n"
            << "declare i32 @txrt_db_begin(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_db_commit(ptr)\n"
            << "declare i32 @txrt_db_rollback(ptr, ptr)\n"
            << "declare i32 @txrt_db_savepoint(ptr, ptr)\n"
            << "declare i32 @txrt_db_rollback_to(ptr, ptr)\n"
            << "declare i32 @txrt_db_release_savepoint(ptr, ptr)\n";
    for (const auto* name : {"int", "float", "bool", "str", "bytes",
                            "decimal", "datetime"})
    {
        const bool domain = std::string_view(name) == "decimal" ||
                            std::string_view(name) == "datetime";
        module_ << "declare i32 @txrt_db_get_" << name
                << "(ptr, i64, ptr, " << (domain ? "ptr, " : "") << "ptr)\n"
                << "declare i32 @txrt_db_as_" << name
                << "(ptr, ptr, " << (domain ? "ptr, " : "") << "ptr)\n";
    }
}

} // namespace tx
