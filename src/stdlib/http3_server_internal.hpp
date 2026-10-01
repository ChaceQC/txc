#pragma once

#include "stdlib/http3_quic.hpp"
#include "stdlib/http3_server.hpp"

#include <nghttp3/nghttp3.h>

#ifdef _WIN32
#include <wincrypt.h>
#endif

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace tx_generated::http3
{

class server_connection;
class server_listener;

struct server_request
{
    std::weak_ptr<server_connection> session;
    std::int64_t stream_id = -1;
    http_request_data value;
    std::size_t header_bytes = 0;
    std::string scheme;
    std::string authority;
    std::string response_body;
    std::size_t response_offset = 0;
    bool complete = false;
    bool closed = false;
    bool delivered = false;
    bool response_started = false;
    bool response_done = false;
    bool body_offered = false;
};

struct server_request_ticket
{
    std::shared_ptr<server_listener> listener;
    std::shared_ptr<server_connection> connection;
    std::shared_ptr<server_request> request;
};

class server_listener : public std::enable_shared_from_this<server_listener>
{
public:
    server_listener(std::string_view host, std::int64_t port,
                    const byte_value& package,
                    const secret::handle& password);
    ~server_listener() noexcept;
    server_listener(const server_listener&) = delete;
    server_listener& operator=(const server_listener&) = delete;

    void queue(server_request_ticket request);
    void discard(const server_connection* connection,
                 std::int64_t stream_id = -1);
    void request_reap() noexcept;
    void report_error(std::exception_ptr error) noexcept;
    server_request_ticket pop(std::int64_t timeout_ms);
    void close() noexcept;
    [[nodiscard]] HQUIC configuration() const noexcept
    {
        return configuration_;
    }

private:
    static QUIC_STATUS QUIC_API on_listener(HQUIC, void*,
                                              QUIC_LISTENER_EVENT*);
    QUIC_STATUS event(QUIC_LISTENER_EVENT* event);
    void reap(std::stop_token token);

    quic_api& library_;
    const QUIC_API_TABLE* api_ = nullptr;
    HQUIC configuration_ = nullptr;
    HQUIC listener_ = nullptr;
#ifdef _WIN32
    HCERTSTORE certificate_store_ = nullptr;
    PCCERT_CONTEXT certificate_ = nullptr;
#endif
    std::mutex mutex_;
    std::condition_variable changed_;
    std::mutex reaper_mutex_;
    std::condition_variable reaper_changed_;
    std::deque<server_request_ticket> completed_;
    std::exception_ptr error_;
    std::vector<std::shared_ptr<server_connection>> connections_;
    std::jthread reaper_;
    bool closed_ = false;
};

class server_connection : public std::enable_shared_from_this<server_connection>
{
public:
    server_connection(std::shared_ptr<server_listener> listener,
                      HQUIC connection);
    ~server_connection() noexcept;
    server_connection(const server_connection&) = delete;
    server_connection& operator=(const server_connection&) = delete;

    void respond(const std::shared_ptr<server_request>& request,
                 const http_response_data& response);
    void close_stream(std::int64_t id) noexcept;
    bool mark_delivered(const std::shared_ptr<server_request>& request) noexcept;
    void close_unclaimed_requests() noexcept;
    void reap_native_handles() noexcept;
    void wait_if_idle_closed() noexcept;
    void shutdown() noexcept;
    void reject_failed_configuration() noexcept;
    [[nodiscard]] bool stopped() const noexcept
    {
        return native_closed_.load();
    }

private:
    struct stream_state
    {
        enum class role
        {
            control,
            encoder,
            decoder,
            peer_request,
            peer_unidirectional
        } kind = role::peer_request;
        enum class close_reason
        {
            active,
            normal,
            peer_abort,
            local_abort,
            connection_closed
        } closure = close_reason::active;
        HQUIC handle = nullptr;
        std::int64_t id = -1;
        std::uint64_t peer_error_code = 0;
        std::uint32_t callbacks_active = 0;
        std::uint32_t native_operations_active = 0;
        std::size_t pending_sends = 0;
        bool started = false;
        bool peer_send_shutdown = false;
        bool send_shutdown_complete = false;
        bool response_fin_submitted = false;
        bool shutdown_complete = false;
        bool abort_requested = false;
        bool close_called = false;
    };
    struct send_state
    {
        std::vector<std::uint8_t> bytes;
        QUIC_BUFFER buffer{};
        std::int64_t stream_id = -1;
        bool fin = false;
    };

    static QUIC_STATUS QUIC_API on_connection(HQUIC, void*,
                                                QUIC_CONNECTION_EVENT*);
    static QUIC_STATUS QUIC_API on_stream(HQUIC, void*, QUIC_STREAM_EVENT*);
    static int on_begin_headers(nghttp3_conn*, std::int64_t, void*, void*);
    static int on_header(nghttp3_conn*, std::int64_t, std::int32_t,
                         nghttp3_rcbuf*, nghttp3_rcbuf*, std::uint8_t,
                         void*, void*);
    static int on_data(nghttp3_conn*, std::int64_t, const std::uint8_t*,
                       std::size_t, void*, void*);
    static int on_end_stream(nghttp3_conn*, std::int64_t, void*, void*);
    static nghttp3_ssize read_body(nghttp3_conn*, std::int64_t,
                                   nghttp3_vec*, std::size_t,
                                   std::uint32_t*, void*, void*);

    void connection_event(QUIC_CONNECTION_EVENT* event);
    void stream_event(stream_state& stream, QUIC_STREAM_EVENT* event);
    void finish_stream_shutdown(stream_state& stream,
                                QUIC_STREAM_EVENT* event) noexcept;
    void release_stream_callback(stream_state& stream) noexcept;
    void fail_stream(stream_state& stream) noexcept;
    void receive(stream_state& stream, const QUIC_BUFFER* buffers,
                 std::uint32_t count, bool fin);
    void open_control_streams();
    void bind_control_streams();
    void pump();
    void capture_error() noexcept;
    void check_error();
    void wait_native_shutdown() noexcept;
    void reap_closed_streams() noexcept;
    void close_native_handles() noexcept;
    std::vector<std::pair<std::string, std::string>> response_fields(
        const http_response_data& response);

    std::weak_ptr<server_listener> listener_;
    const QUIC_API_TABLE* api_ = nullptr;
    HQUIC connection_ = nullptr;
    nghttp3_conn* h3_ = nullptr;
    std::mutex mutex_;
    std::condition_variable changed_;
    std::vector<std::unique_ptr<stream_state>> streams_;
    std::unordered_map<std::int64_t, HQUIC> stream_handles_;
    std::unordered_map<std::int64_t, std::shared_ptr<server_request>> requests_;
    std::int64_t control_id_ = -1;
    std::int64_t encoder_id_ = -1;
    std::int64_t decoder_id_ = -1;
    std::exception_ptr error_;
    std::atomic<bool> shutdown_ = false;
    std::atomic<bool> shutdown_requested_ = false;
    std::uint32_t connection_callbacks_active_ = 0;
    std::uint32_t connection_operations_active_ = 0;
    std::uint32_t stream_callbacks_active_ = 0;
    std::uint32_t stream_closes_active_ = 0;
    bool native_close_started_ = false;
    bool listener_closed_ = false;
    bool bound_ = false;
    bool connection_shutdown_complete_ = false;
    std::atomic<bool> native_closed_ = false;
};

} // namespace tx_generated::http3
