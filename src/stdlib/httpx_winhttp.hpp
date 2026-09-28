#pragma once

#include "stdlib/httpx.hpp"

#include <string>

namespace tx_generated::httpx_winhttp
{

std::wstring request_headers(const network::header_map& headers);
http_response_data response_metadata(HINTERNET request);
void require_http2_protocol(HINTERNET request);

} // namespace tx_generated::httpx_winhttp
