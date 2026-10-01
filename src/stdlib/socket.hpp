#pragma once

#include "stdlib/network_common.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>


namespace tx_generated::socket
{

enum class resource_kind
{
    tcp_listener,
    tcp_stream,
    udp_socket
};

struct state
{
    resource_kind kind;
    std::mutex mutex;
    std::mutex read_mutex;
    std::mutex write_mutex;
    std::shared_ptr<network::socket_handle> native;
    std::atomic<bool> closed = false;
    std::atomic<bool> read_closed = false;
    std::atomic<bool> write_closed = false;
    std::atomic<bool> remote_eof = false;
};

struct resource
{
    std::int64_t id;
    std::shared_ptr<state> value;
};

struct read_result
{
    std::vector<std::uint8_t> data;
    bool eof = false;
};

struct datagram
{
    std::vector<std::uint8_t> data;
    std::string host;
    std::int64_t port = 0;
    bool truncated = false;
};

using cancellation_probe = std::function<void()>;
using address_list = std::unique_ptr<addrinfo, decltype(&freeaddrinfo)>;

resource register_socket(resource_kind kind, network::socket_handle native);
std::shared_ptr<state> get(std::int64_t id, resource_kind kind);
void close(std::int64_t id, resource_kind kind);
std::shared_ptr<network::socket_handle> take_tcp(std::int64_t id);
std::shared_ptr<network::socket_handle> native_socket(
    const std::shared_ptr<state>& value);
address_list numeric_addresses(std::string_view ip, std::int64_t port,
                               int socktype, int protocol, bool passive);
network::socket_handle create_socket(const addrinfo& address);

std::int64_t local_port(const std::shared_ptr<state>& value);
void check_timeout(std::int64_t timeout_ms);
void check_port(std::int64_t port, bool allow_zero);
void wait_ready(const std::shared_ptr<state>& value, SOCKET native,
                bool write, std::chrono::steady_clock::time_point deadline,
                const cancellation_probe& cancellation);
[[noreturn]] void socket_error(std::string_view action);

resource listen_tcp(std::string_view ip, std::int64_t port,
                    std::int64_t backlog);
resource connect_tcp(std::string_view ip, std::int64_t port,
                     std::int64_t timeout_ms,
                     const cancellation_probe& cancellation = {});
resource accept_tcp(const std::shared_ptr<state>& listener,
                    std::int64_t timeout_ms,
                    const cancellation_probe& cancellation = {});
read_result read(const std::shared_ptr<state>& peer,
                 std::int64_t max_bytes, std::int64_t timeout_ms,
                 const cancellation_probe& cancellation = {});
std::int64_t write(const std::shared_ptr<state>& peer,
                   std::string_view data, std::int64_t timeout_ms,
                   const cancellation_probe& cancellation = {});
void shutdown_read(const std::shared_ptr<state>& peer);
void shutdown_write(const std::shared_ptr<state>& peer);

resource bind_udp(std::string_view ip, std::int64_t port);
std::int64_t send_to(const std::shared_ptr<state>& endpoint,
                     std::string_view ip, std::int64_t port,
                     std::string_view data, std::int64_t timeout_ms,
                     const cancellation_probe& cancellation = {});
datagram receive_from(const std::shared_ptr<state>& endpoint,
                      std::int64_t max_bytes, std::int64_t timeout_ms,
                      const cancellation_probe& cancellation = {});

} // namespace tx_generated::socket
