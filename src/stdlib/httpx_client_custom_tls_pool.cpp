#include "stdlib/httpx_client_custom_tls_state.hpp"

namespace tx_generated::httpx_custom_tls
{

session_pool::session_pool(
    std::shared_ptr<const httpx_client_tls::settings> tls_settings,
    HINTERNET proxy_session, std::string_view proxy_url,
    std::int64_t max_connections, bool decompress, bool allow_http2)
    : value_(std::make_shared<state>())
{
    value_->tls_settings = std::move(tls_settings);
    value_->proxy_session = proxy_session;
    value_->proxy_url = std::string(proxy_url);
    value_->max_connections = max_connections;
    value_->decompress = decompress;
    value_->allow_http2 = allow_http2;
}

session_pool::~session_pool() noexcept
{
    close();
}

void session_pool::close() noexcept
{
    if (value_)
    {
        value_->close();
    }
}

} // namespace tx_generated::httpx_custom_tls
