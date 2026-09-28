#pragma once

#include "stdlib/secret.hpp"
#include "stdlib/vector.hpp"

#include <memory>
#include <string_view>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#include <winhttp.h>

namespace tx_generated::httpx_client_tls
{

struct settings;

std::shared_ptr<const settings> create(const bytes_vector& anchors,
    bool include_system, const byte_value& package,
    const secret::handle& password);
void configure(HINTERNET request, const settings& value);
void verify(HINTERNET request, std::string_view hostname,
    const settings& value);

} // namespace tx_generated::httpx_client_tls
