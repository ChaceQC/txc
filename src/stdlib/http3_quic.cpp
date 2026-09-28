#include "stdlib/http3_quic.hpp"

#include "stdlib/network_common.hpp"

#include <string>

namespace tx_generated::http3
{

void require_quic(QUIC_STATUS status, std::string_view action)
{
    if (QUIC_FAILED(status))
    {
        network::fail("operation_failed", std::string(action) +
            "失败，MsQuic 状态码 " + std::to_string(status));
    }
}

quic_api::quic_api()
{
    module_ = LoadLibraryW(L"msquic.dll");
    if (!module_)
    {
        network::fail("unsupported_protocol", "HTTP/3 需要随工具链交付的 MsQuic 运行库");
    }
    try
    {
        const auto open = reinterpret_cast<MsQuicOpenVersionFn>(
            GetProcAddress(module_, "MsQuicOpenVersion"));
        close_ = reinterpret_cast<MsQuicCloseFn>(
            GetProcAddress(module_, "MsQuicClose"));
        if (!open || !close_)
        {
            network::fail("unsupported_protocol", "MsQuic 运行库缺少版本 2 接口");
        }
        require_quic(open(QUIC_API_VERSION_2,
            reinterpret_cast<const void**>(&api_)), "加载 MsQuic API");
        QUIC_REGISTRATION_CONFIG config{"tx-http3",
                                        QUIC_EXECUTION_PROFILE_LOW_LATENCY};
        require_quic(api_->RegistrationOpen(&config, &registration_),
                     "注册 QUIC 应用");
    }
    catch (...)
    {
        if (api_)
        {
            close_(api_);
        }
        FreeLibrary(module_);
        throw;
    }
}

quic_api::~quic_api() noexcept
{
    if (registration_)
    {
        api_->RegistrationClose(registration_);
    }
    if (api_)
    {
        close_(api_);
    }
    if (module_)
    {
        FreeLibrary(module_);
    }
}

quic_api& quic()
{
    static quic_api value;
    return value;
}

} // namespace tx_generated::http3
