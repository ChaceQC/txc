#pragma once

#include "stdlib/tls_stream.hpp"

namespace tx_generated::tls_abi
{

tls::client_options checked_client(const void* config);
tls::server_options checked_server(const void* config);

} // namespace tx_generated::tls_abi
