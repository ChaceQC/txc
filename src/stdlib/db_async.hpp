#pragma once

#include "stdlib/db.hpp"
#include "stdlib/db_operation.hpp"

namespace tx_generated
{

struct db_async_request
{
    db_pool pool;
    std::string sql;
    std::vector<db_value> parameters;
    std::int64_t max_rows = 0;
    std::int64_t max_bytes = 0;
    bool query = false;
    db_operation_context operation;
};

using db_async_result = std::variant<db_execution, std::vector<db_row>>;
std::size_t db_value_size(const db_value& value);
db_async_result db_run_async(const db_async_request& request);

} // namespace tx_generated
