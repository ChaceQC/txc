#pragma once

#include "stdlib/socket.hpp"
#include "stdlib/tls.hpp"
#include "stdlib/tls_material.hpp"
#include "stdlib/vector.hpp"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace tx_generated::tls
{

struct trust_options
{
    bool system_roots = false;
    bytes_vector anchors;
};

struct client_options
{
    std::string hostname;
    trust_options trust;
    std::int64_t identity_id = 0;
    bool allow_no_alpn = false;
};

struct server_options
{
    std::int64_t identity_id = 0;
    trust_options trust;
    bool require_client_identity = false;
};

void validate_alpn(const std::vector<std::string>& protocols,
                   std::int64_t timeout_ms);

class secure_connection
{
public:
    secure_connection(std::shared_ptr<network::socket_handle> native,
                      client_options options,
                      std::vector<std::string> protocols,
                      std::int64_t timeout_ms);
    secure_connection(std::shared_ptr<network::socket_handle> native,
                      server_options options,
                      std::vector<std::string> protocols,
                      std::int64_t timeout_ms);
    ~secure_connection() noexcept;
    secure_connection(const secure_connection&) = delete;
    secure_connection& operator=(const secure_connection&) = delete;

    socket::read_result read(std::int64_t max_bytes,
                             std::int64_t timeout_ms);
    std::int64_t write(std::string_view data, std::int64_t timeout_ms);
    std::string negotiated_alpn() const;
    void close(std::int64_t timeout_ms);
    [[nodiscard]] bool closed() const noexcept;

private:
    void initialize(std::int64_t timeout_ms);
    void load_identity();
    void handshake(std::int64_t timeout_ms);
    void verify_peer();
    void wait_io(bool write, std::chrono::steady_clock::time_point deadline);
    void release() noexcept;
    static int tls_send(void* context, const unsigned char* data,
                        std::size_t size);
    static int tls_recv(void* context, unsigned char* data,
                        std::size_t size);

    mutable std::mutex mutex_;
    std::shared_ptr<network::socket_handle> native_;
    bool server_ = false;
    bool require_client_identity_ = false;
    bool allow_no_alpn_ = false;
    bool peer_eof_ = false;
    bool released_ = false;
    std::atomic<bool> closed_ = false;
    std::int64_t identity_id_ = 0;
    std::string hostname_;
    std::string selected_alpn_;
    trust_options trust_;
    std::vector<std::string> protocols_;
    std::vector<const char*> protocol_pointers_;
    std::shared_ptr<const identity_state> identity_owner_;
    std::shared_ptr<identity_material> identity_material_;
    mbedtls_ssl_config config_{};
    mbedtls_ssl_context ssl_{};
};

struct stream_resource
{
    std::int64_t id;
    std::shared_ptr<secure_connection> value;
};

stream_resource register_stream(std::shared_ptr<secure_connection> value);
std::shared_ptr<secure_connection> get_stream(std::int64_t id);
void close_stream(std::int64_t id, std::int64_t timeout_ms);

} // namespace tx_generated::tls
