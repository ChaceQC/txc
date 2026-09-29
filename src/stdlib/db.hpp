#pragma once

#include "stdlib/bytes.hpp"
#include "stdlib/error.hpp"
#include "stdlib/secret.hpp"

#include <cstdint>
#include <any>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace tx_generated
{

enum class db_value_kind
{
    null,
    integer,
    floating,
    boolean,
    text,
    binary,
    decimal,
    datetime
};

struct db_value
{
    db_value_kind kind = db_value_kind::null;
    std::variant<std::monostate, std::int64_t, double, bool,
                 std::string, byte_value> data;
};

struct db_row_data
{
    std::vector<std::string> names;
    std::vector<db_value> values;
};

struct db_connection_state;
struct db_statement_state;
struct db_cursor_state;
struct db_transaction_state;
struct db_pool_state;
using db_connection = std::shared_ptr<db_connection_state>;
using db_statement = std::shared_ptr<db_statement_state>;
using db_cursor = std::shared_ptr<db_cursor_state>;
using db_transaction = std::shared_ptr<db_transaction_state>;
using db_row = std::shared_ptr<const db_row_data>;
using db_pool = std::shared_ptr<db_pool_state>;

struct db_options
{
    std::string driver = "sqlite";
    std::string path;
    bool read_only = false;
    bool wal = false;
    std::int64_t busy_timeout_ms = 5000;
    std::int64_t max_value_bytes = 1048576;
};

struct db_execution
{
    std::int64_t affected_rows = 0;
    std::optional<std::int64_t> insert_id;
};

struct db_postgres_options
{
    std::string host;
    std::int64_t port = 5432;
    std::string database;
    std::string user;
    std::string ca_file;
    std::int64_t connect_timeout_ms = 5000;
    std::int64_t operation_timeout_ms = 30000;
    std::int64_t max_value_bytes = 1048576;
    bool read_only = false;
};

db_connection db_open_postgres(const db_postgres_options& options,
                               const secret::handle& password);
std::string db_sqlstate(const db_connection& connection);
std::string db_transaction_status(const db_connection& connection);
db_pool db_make_pool(const db_options& options, std::int64_t capacity,
                     std::int64_t waiters);
db_pool db_make_postgres_pool(const db_postgres_options& options,
    const secret::handle& password, std::int64_t capacity, std::int64_t waiters);
db_connection db_acquire(const db_pool& pool, std::int64_t timeout_ms);
bool db_release(const db_connection& connection);
bool db_close_pool(const db_pool& pool);

[[noreturn]] void db_fail(const char* code, const char* message);
std::string_view db_runtime_type(const std::any& value);
bool db_type_matches(const std::any& value, std::string_view name);
std::string_view db_kind_name(const db_value& value);
void db_validate_value(const db_value& value, std::int64_t limit);
const db_value& db_column(const db_row& row, std::int64_t index);
std::int64_t db_column_index(const db_row& row, std::string_view name);
std::optional<std::int64_t> db_as_int(const db_value& value);
std::optional<double> db_as_float(const db_value& value);
std::optional<bool> db_as_bool(const db_value& value);
std::optional<std::string> db_as_str(const db_value& value);
std::optional<byte_value> db_as_bytes(const db_value& value);
std::optional<std::string> db_domain_text(const db_value& value,
                                         db_value_kind kind);

db_connection db_open(const db_options& options);
bool db_close(const db_connection& connection);
std::int64_t db_extended_error(const db_connection& connection);
db_statement db_prepare(const db_connection& connection, std::string_view sql);
std::int64_t db_parameter_count(const db_statement& statement);
std::int64_t db_parameter_index(const db_statement& statement,
                                std::string_view name);
void db_bind(const db_statement& statement, std::int64_t index,
             const db_value& value);
void db_bind_all(const db_statement& statement,
                 const std::vector<db_value>& values);
void db_clear_bindings(const db_statement& statement);
db_execution db_execute(const db_statement& statement);
db_cursor db_query(const db_statement& statement);
db_row db_next(const db_cursor& cursor);
bool db_close_statement(const db_statement& statement);
bool db_close_cursor(const db_cursor& cursor);
db_transaction db_begin(const db_connection& connection, std::string_view mode);
void db_commit(const db_transaction& transaction);
bool db_rollback(const db_transaction& transaction);
void db_savepoint(const db_transaction& transaction, std::string_view name);
void db_rollback_to(const db_transaction& transaction, std::string_view name);
void db_release_savepoint(const db_transaction& transaction,
                          std::string_view name);

} // namespace tx_generated
