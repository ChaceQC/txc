#pragma once

#include "stdlib/http3_client.hpp"
#include "stdlib/http3_quic.hpp"

#include <nghttp3/nghttp3.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace tx_generated::http3
{

class client_connection
{
public:
    client_connection(std::int64_t timeout_ms, bool binary,
                      const std::vector<byte_value>& trust_anchors,
                      const cancel_token* token);
    ~client_connection() noexcept;
    client_connection(const client_connection&) = delete;
    client_connection& operator=(const client_connection&) = delete;

    http_response_data send(std::string_view method,
                            std::string_view url,
                            const network::header_map& headers,
                            std::string_view body);

private:
    struct stream_state
    {
        client_connection* owner = nullptr;
        HQUIC handle = nullptr;
        std::int64_t id = -1;
        bool started = false;
    };

    struct send_state
    {
        std::vector<std::uint8_t> bytes;
        QUIC_BUFFER buffer{};
        std::int64_t stream_id = -1;
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
    void validate_peer(QUIC_CONNECTION_EVENT* event);
    void stream_event(stream_state& stream, QUIC_STREAM_EVENT* event);
    void receive(stream_state& stream, const QUIC_BUFFER* buffers,
                 std::uint32_t count, bool fin);
    void pump();
    void capture_error() noexcept;
    void check_error();
    void check_cancellation();
    void wait_connected();
    stream_state& open_stream(QUIC_STREAM_OPEN_FLAGS flags);
    void open_http3_streams();
    void submit_request(std::string_view method,
                        const network::parsed_url& address,
                        const network::header_map& headers);
    void await_response();

    quic_api& library_;
    const QUIC_API_TABLE* api_ = nullptr;
    HQUIC configuration_ = nullptr;
    HQUIC connection_ = nullptr;
    nghttp3_conn* h3_ = nullptr;
    std::mutex mutex_;
    std::condition_variable changed_;
    std::vector<std::unique_ptr<stream_state>> streams_;
    std::unordered_map<std::int64_t, HQUIC> stream_handles_;
    std::chrono::steady_clock::time_point deadline_;
    std::exception_ptr error_;
    http_response_data response_;
    std::string body_;
    std::string method_;
    std::string hostname_;
    std::vector<byte_value> trust_anchors_;
    std::thread validation_thread_;
    std::shared_ptr<cancellation_state> cancellation_;
    std::size_t header_bytes_ = 0;
    std::int64_t response_stream_ = -1;
    bool body_offered_ = false;
    bool binary_ = false;
    bool connected_ = false;
    bool shutdown_ = false;
    bool response_done_ = false;
};

} // namespace tx_generated::http3
