#pragma once

#include "backend/cpp/task_runtime_internal.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/socket.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>

namespace tx_generated::socket_async
{

struct operation;
using step_result = std::optional<task_result>;
using step_function = std::function<step_result(operation&)>;

struct operation
{
    std::shared_ptr<socket::state> state;
    std::shared_ptr<network::socket_handle> native;
    std::shared_ptr<task_scope_state> scope;
    std::shared_ptr<cancellation_state> token;
    std::shared_ptr<task_state> child;
    std::chrono::steady_clock::time_point deadline;
    std::int64_t timeout_ms = 0;
    bool write = false;
    bool initialized = false;
    bool finished = false;
    step_function start;
    step_function ready;
};

void submit(std::shared_ptr<operation> value);

std::shared_ptr<operation> make_connect(std::string ip, std::int64_t port,
    std::int64_t timeout_ms, std::string result_name);
std::shared_ptr<operation> make_accept(std::shared_ptr<socket::state> listener,
    std::int64_t timeout_ms, std::string result_name);
std::shared_ptr<operation> make_read(std::shared_ptr<socket::state> peer,
    std::int64_t max_bytes, std::int64_t timeout_ms, std::string result_name);
std::shared_ptr<operation> make_write(std::shared_ptr<socket::state> peer,
    byte_value bytes, std::int64_t timeout_ms);
std::shared_ptr<operation> make_send_to(std::shared_ptr<socket::state> endpoint,
    std::string ip, std::int64_t port, byte_value bytes,
    std::int64_t timeout_ms);
std::shared_ptr<operation> make_receive_from(
    std::shared_ptr<socket::state> endpoint, std::int64_t max_bytes,
    std::int64_t timeout_ms, std::string result_name);

} // namespace tx_generated::socket_async
