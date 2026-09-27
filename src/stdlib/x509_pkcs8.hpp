#pragma once

#include "stdlib/secret.hpp"

#ifdef _WIN32
#include <windows.h>
#include <ncrypt.h>

namespace tx_generated::x509
{

secret::handle export_private_key(NCRYPT_KEY_HANDLE key);

} // namespace tx_generated::x509
#endif
