#pragma once

#ifdef _WIN32
#ifndef _Pre_defensive_
#define _Pre_defensive_
#endif
#include <winsock2.h>
#include <windows.h>
#pragma push_macro("WINAPI_FAMILY")
#undef WINAPI_FAMILY
#define WINAPI_FAMILY WINAPI_FAMILY_GAMES
#include <msquic.h>
#pragma pop_macro("WINAPI_FAMILY")
#else
#include <msquic.h>
#endif

#include <string_view>

namespace tx_generated::http3
{

class quic_api
{
public:
    quic_api();
    ~quic_api() noexcept;
    quic_api(const quic_api&) = delete;
    quic_api& operator=(const quic_api&) = delete;

    [[nodiscard]] const QUIC_API_TABLE* get() const noexcept
    {
        return api_;
    }
    [[nodiscard]] HQUIC registration() const noexcept
    {
        return registration_;
    }

private:
#ifdef _WIN32
    HMODULE module_ = nullptr;
#else
    void* module_ = nullptr;
#endif
    MsQuicCloseFn close_ = nullptr;
    const QUIC_API_TABLE* api_ = nullptr;
    HQUIC registration_ = nullptr;
};

quic_api& quic();
void require_quic(QUIC_STATUS status, std::string_view action);

} // namespace tx_generated::http3
