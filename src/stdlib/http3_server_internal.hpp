#pragma once

#include "stdlib/http3_quic.hpp"
#include "stdlib/http3_server.hpp"

#include <nghttp3/nghttp3.h>

#include <wincrypt.h>

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

struct server_request
{
    std::shared_ptr<server_connection> session;
    std::int64_t stream_id = -1;
    http_request_data value;
    std::size_t header_bytes = 0;
    std::string scheme;
    std::string authority;
    std::string response_body;
    std::size_t response_offset = 0;
    bool complete = false;
    bool response_started = false;
    bool response_done = false;
    bool body_offered = false;
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

    void queue(std::shared_ptr<server_request> request);
    void report_error(std::exception_ptr error) noexcept;
    std::shared_ptr<server_request> pop(std::int64_t timeout_ms);
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
    HCERTSTORE certificate_store_ = nullptr;
    PCCERT_CONTEXT certificate_ = nullptr;
    std::mutex mutex_;
    std::condition_variable changed_;
    std::deque<std::shared_ptr<server_request>> completed_;
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
    void reject_failed_configuration() noexcept;
    [[nodiscard]] bool stopped() const noexcept
    {
        return shutdown_;
    }

private:
    struct stream_state
    {
        enum class role
        {
            control,
            encoder,
            decoder,
            peer
        } kind = role::peer;
        server_connection* owner = nullptr;
        HQUIC handle = nullptr;
        std::int64_t id = -1;
        bool started = false;
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
    void receive(stream_state& stream, const QUIC_BUFFER* buffers,
                 std::uint32_t count, bool fin);
    void open_control_streams();
    void bind_control_streams();
    void pump();
    void capture_error() noexcept;
    void check_error();
    std::vector<std::pair<std::string, std::string>> response_fields(
        const http_response_data& response);

    std::shared_ptr<server_listener> listener_;
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
    bool bound_ = false;
};

} // namespace tx_generated::http3
