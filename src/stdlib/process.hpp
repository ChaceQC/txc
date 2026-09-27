#pragma once

#include "stdlib/cancellation.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace tx_generated
{

struct process_child_state;
struct process_pipe_state;
using process_child = std::shared_ptr<process_child_state>;
using process_pipe = std::shared_ptr<process_pipe_state>;

struct process_options
{
    std::string executable;
    std::vector<std::string> args;
    std::string cwd;
    std::vector<std::string> env;
    bool clear_env = false;
    std::string stdin_mode = "pipe";
    std::string stdout_mode = "pipe";
    std::string stderr_mode = "pipe";
    bool new_process_group = false;
};

struct process_status
{
    std::string state = "running";
    std::int64_t exit_code = 0;
};

process_child process_spawn(const process_options& options);
std::int64_t process_id(const process_child& child);
process_status process_wait(const process_child& child, std::int64_t timeout_ms,
    const std::shared_ptr<cancellation_state>& cancellation = {});
void process_terminate(const process_child& child);
void process_kill(const process_child& child);
void process_close(const process_child& child);

} // namespace tx_generated
