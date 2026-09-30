#pragma once

#include "stdlib/cancellation.hpp"
#include "stdlib/process_internal.hpp"

#include <any>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace tx_generated
{

struct ipc_listener_state
{
    std::mutex mutex;
#ifdef _WIN32
    std::wstring name;
#else
    std::string name;
#endif
    process_detail::native_handle pending;
    std::size_t max_bytes = 0;
    bool closed = false;
};

struct ipc_stream_state
{
    ~ipc_stream_state();

    std::mutex read_mutex;
    std::mutex write_mutex;
    process_detail::native_handle native;
    process_pipe reader;
    process_pipe writer;
    std::vector<std::uint8_t> pending;
    std::size_t max_bytes = 0;
    std::int64_t local_session = 0;
    std::int64_t peer_session = 0;
    bool has_peer_session = false;
    bool closed = false;
};

using ipc_listener = std::shared_ptr<ipc_listener_state>;
using ipc_stream = std::shared_ptr<ipc_stream_state>;
using ipc_clock = std::chrono::steady_clock;

enum class ipc_transfer_state
{
    data,
    eof,
    timeout,
    cancelled,
    error
};

struct ipc_transfer
{
    ipc_transfer_state state = ipc_transfer_state::data;
    std::size_t count = 0;
    std::string error_code;
};

struct ipc_message
{
    std::string state;
    std::int64_t version = 0;
    std::int64_t session = 0;
    std::any value;
    std::int64_t received = 0;
    std::string error_code;
};

struct ipc_send_result
{
    std::string state;
    std::int64_t written = 0;
    std::string error_code;
};

ipc_listener ipc_listen(std::string_view name, std::int64_t max_bytes);
ipc_stream ipc_accept(const ipc_listener& listener, std::int64_t timeout_ms,
                      const std::shared_ptr<cancellation_state>& token);
ipc_stream ipc_connect(std::string_view name, std::int64_t max_bytes,
                       std::int64_t timeout_ms,
                       const std::shared_ptr<cancellation_state>& token);
ipc_stream ipc_from_process_pipes(const process_pipe& reader,
                                  const process_pipe& writer,
                                  std::int64_t max_bytes);
bool ipc_close_listener(const ipc_listener& listener);
bool ipc_close(const ipc_stream& stream);

ipc_clock::time_point ipc_deadline(std::int64_t timeout_ms);
bool ipc_cancelled(const std::shared_ptr<cancellation_state>& token);
ipc_transfer ipc_read_some(const ipc_stream& stream, std::uint8_t* data,
                           std::size_t size, ipc_clock::time_point deadline,
                           const std::shared_ptr<cancellation_state>& token);
ipc_transfer ipc_write_some(const ipc_stream& stream, const std::uint8_t* data,
                            std::size_t size, ipc_clock::time_point deadline,
                            const std::shared_ptr<cancellation_state>& token);
ipc_message ipc_receive(const ipc_stream& stream, std::int64_t timeout_ms,
                        const std::shared_ptr<cancellation_state>& token);
ipc_send_result ipc_send(const ipc_stream& stream, std::int64_t version,
                         const std::any& value, std::int64_t timeout_ms,
                         const std::shared_ptr<cancellation_state>& token);

} // namespace tx_generated
