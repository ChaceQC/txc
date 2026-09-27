#pragma once

#include "stdlib/process.hpp"
#include "stdlib/vector.hpp"

namespace tx_generated
{

struct process_chunk
{
    byte_value data;
    std::string state;
};

struct process_write_result
{
    std::int64_t written = 0;
    std::string state;
};

struct process_limits
{
    byte_value input;
    std::int64_t timeout_ms = -1;
    std::int64_t max_output_bytes = 1024 * 1024;
    std::string stop_action = "keep";
};

struct process_completed
{
    process_child child;
    process_status status;
    byte_value stdout_data;
    byte_value stderr_data;
    std::string reason = "completed";
    std::int64_t input_written = 0;
};

process_pipe process_get_pipe(const process_child& child, unsigned index);
void process_close_pipe(const process_pipe& pipe);
process_chunk process_read_pipe(const process_pipe& pipe, std::int64_t max_bytes,
    std::int64_t timeout_ms);
process_write_result process_write_pipe(const process_pipe& pipe,
    const byte_value& data, std::int64_t timeout_ms);
process_completed process_run(const process_options& options,
    const process_limits& limits,
    const std::shared_ptr<cancellation_state>& cancellation = {});

} // namespace tx_generated
