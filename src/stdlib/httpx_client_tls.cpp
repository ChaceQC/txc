#include "stdlib/httpx_client_tls.hpp"

#include "stdlib/encoding.hpp"
#include "stdlib/error.hpp"
#include "stdlib/httpx.hpp"
#include "stdlib/tls_stream.hpp"
#include "stdlib/tls.hpp"
#include "stdlib/x509_win.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <ncrypt.h>

namespace tx_generated::httpx_client_tls
{
namespace
{

std::size_t password_character_count(std::span<const std::uint8_t> raw)
{
    if (raw.size() > 4096 || std::find(raw.begin(), raw.end(), 0) != raw.end())
    {
        network::fail("invalid_argument", "客户端证书密码长度无效");
    }
    if (raw.empty())
    {
        return 0;
    }
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        reinterpret_cast<const char*>(raw.data()),
        static_cast<int>(raw.size()), nullptr, 0);
    if (count <= 0)
    {
        network::fail("invalid_argument", "客户端证书密码不是有效 UTF-8");
    }
    return static_cast<std::size_t>(count);
}

class sensitive_wide_buffer
{
public:
    explicit sensitive_wide_buffer(std::size_t characters)
        : value_(characters + 1, L'\0')
    {
    }

    ~sensitive_wide_buffer()
    {
        if (!value_.empty())
        {
            SecureZeroMemory(value_.data(), value_.size() * sizeof(wchar_t));
        }
    }

    sensitive_wide_buffer(const sensitive_wide_buffer&) = delete;
    sensitive_wide_buffer& operator=(const sensitive_wide_buffer&) = delete;

    wchar_t* data() noexcept
    {
        return value_.data();
    }

    const wchar_t* c_str() const noexcept
    {
        return value_.data();
    }

    std::size_t characters() const noexcept
    {
        return value_.size() - 1;
    }

private:
    std::vector<wchar_t> value_;
};

class password_buffer
{
public:
    explicit password_buffer(const secret::handle& source)
        : value_(password_character_count(source->view()))
    {
        const auto raw = source->view();
        if (raw.empty())
        {
            return;
        }
        const auto characters = value_.characters();
        const int converted = MultiByteToWideChar(CP_UTF8,
            MB_ERR_INVALID_CHARS,
            reinterpret_cast<const char*>(raw.data()),
            static_cast<int>(raw.size()), value_.data(),
            static_cast<int>(characters));
        if (converted <= 0 ||
            static_cast<std::size_t>(converted) != characters)
        {
            network::fail("invalid_argument", "客户端证书密码不是有效 UTF-8");
        }
    }

    password_buffer(const password_buffer&) = delete;
    password_buffer& operator=(const password_buffer&) = delete;

    const wchar_t* data() const noexcept
    {
        return value_.c_str();
    }

private:
    sensitive_wide_buffer value_;
};

} // namespace

struct settings
{
    struct stored_key
    {
        std::wstring provider;
        std::wstring container;
        DWORD provider_type = 0;
        DWORD flags = 0;
        bool cng = true;
    };

    bytes_vector anchors;
    bool include_system = true;
    std::shared_ptr<void> client_store;
    std::shared_ptr<const CERT_CONTEXT> client_certificate;
    std::vector<stored_key> stored_keys;
    std::int64_t identity_id = 0;

    ~settings()
    {
        tls::close_identity(identity_id);
        for (const auto& item : stored_keys)
        {
            if (!item.cng)
            {
                CryptAcquireContextW(nullptr, item.container.c_str(),
                    item.provider.c_str(), item.provider_type,
                    CRYPT_DELETEKEYSET | (item.flags & CRYPT_MACHINE_KEYSET));
                continue;
            }
            NCRYPT_PROV_HANDLE provider = 0;
            NCRYPT_KEY_HANDLE key = 0;
            if (NCryptOpenStorageProvider(&provider, item.provider.c_str(), 0) ==
                    ERROR_SUCCESS)
            {
                if (NCryptOpenKey(provider, &key, item.container.c_str(), 0, 0) ==
                        ERROR_SUCCESS)
                {
                    NCryptDeleteKey(key, 0);
                }
                NCryptFreeObject(provider);
            }
        }
    }
};

std::shared_ptr<const settings> create(const bytes_vector& anchors,
    bool include_system, const byte_value& package,
    const secret::handle& password)
{
    const auto& roots = anchors.data().values;
    if ((!include_system && roots.empty()) || roots.size() > 64)
    {
        network::fail("invalid_argument", "HTTP 信任锚配置无效");
    }
    auto result = std::make_shared<settings>();
    result->anchors = anchors.copy();
    result->include_system = include_system;
    for (const auto& item : roots)
    {
        (void)x509::parse_der(item);
    }
    if (!roots.empty())
    {
        if (!package->empty())
        {
            if (package->size() > x509::max_input_bytes)
            {
                network::fail("size_limit", "客户端证书包超过 16 MiB");
            }
            result->identity_id = tls::import_identity(package, password);
        }
        return result;
    }
    if (package->empty())
    {
        return result;
    }
    if (package->size() > x509::max_input_bytes)
    {
        network::fail("size_limit", "客户端证书包超过 16 MiB");
    }
    CRYPT_DATA_BLOB blob{static_cast<DWORD>(package->size()),
                         const_cast<BYTE*>(package->data())};
    if (!PFXIsPFXBlob(&blob))
    {
        network::fail("invalid_argument", "客户端证书不是 PKCS#12 数据包");
    }
    constexpr DWORD flags = PKCS12_ALWAYS_CNG_KSP | CRYPT_USER_KEYSET;
    HCERTSTORE store = nullptr;
    {
        const password_buffer converted(password);
        store = PFXImportCertStore(&blob, converted.data(), flags);
    }
    if (!store)
    {
        network::fail("invalid_argument", "客户端证书或密码无效");
    }
    result->client_store.reset(store, [](void* value)
    {
        CertCloseStore(value, 0);
    });
    x509::certificate_cursor cursor;
    while (const auto* current = cursor.next(store))
    {
        DWORD size = 0;
        const bool has_provider = CertGetCertificateContextProperty(current,
            CERT_KEY_PROV_INFO_PROP_ID, nullptr, &size) != 0;
        if (has_provider)
        {
            std::vector<std::uint8_t> buffer(size);
            if (!CertGetCertificateContextProperty(current,
                    CERT_KEY_PROV_INFO_PROP_ID, buffer.data(), &size))
            {
                network::fail("operation_failed", "读取客户端证书密钥关联失败");
            }
            if (size < sizeof(CRYPT_KEY_PROV_INFO))
            {
                network::fail("operation_failed", "客户端证书密钥关联数据无效");
            }
            const auto* info = reinterpret_cast<const CRYPT_KEY_PROV_INFO*>(
                buffer.data());
            if (info->pwszProvName && info->pwszContainerName)
            {
                result->stored_keys.push_back({info->pwszProvName,
                    info->pwszContainerName, info->dwProvType, info->dwFlags,
                    info->dwKeySpec == CERT_NCRYPT_KEY_SPEC});
            }
        }
        else if (!CertGetCertificateContextProperty(current,
                     CERT_KEY_CONTEXT_PROP_ID, nullptr, &size))
        {
            continue;
        }
        if (result->client_certificate)
        {
            network::fail("invalid_argument", "客户端证书包含多个私钥身份");
        }
        result->client_certificate.reset(
            CertDuplicateCertificateContext(current),
            &CertFreeCertificateContext);
    }
    if (!result->client_certificate)
    {
        network::fail("invalid_argument", "客户端证书没有私钥身份");
    }
    return result;
}

bool has_custom_anchors(const settings& value) noexcept
{
    return !value.anchors.data().values.empty();
}

std::shared_ptr<tls::secure_connection> connect_custom(
    const settings& value, std::shared_ptr<network::socket_handle> socket,
    std::string_view hostname, const std::vector<std::string>& protocols,
    std::int64_t timeout_ms, bool allow_no_alpn)
{
    tls::client_options options;
    options.hostname = std::string(hostname);
    options.allow_no_alpn = allow_no_alpn;
    options.trust.system_roots = value.include_system;
    options.trust.anchors = value.anchors;
    options.identity_id = value.identity_id;
    try
    {
        return std::make_shared<tls::secure_connection>(std::move(socket),
            std::move(options), protocols, timeout_ms);
    }
    catch (const runtime_failure& failure)
    {
        if (failure.error().kind == tx::error_kind::security)
        {
            if (failure.error().code == "alpn_mismatch")
            {
                network::fail("protocol_error",
                    "HTTPS 服务端未协商请求所需的 ALPN 协议");
            }
            network::fail("security_error", failure.error().message);
        }
        throw;
    }
}

void configure(HINTERNET request, const settings& value)
{
    if (!value.anchors.data().values.empty())
    {
        network::fail("security_error",
            "自定义 CA 请求必须使用握手阶段验证的 TLS 连接");
    }
    if (value.client_certificate &&
        !WinHttpSetOption(request, WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
            const_cast<CERT_CONTEXT*>(value.client_certificate.get()),
            sizeof(CERT_CONTEXT)))
    {
        network::http_failure("配置 HTTP 客户端证书");
    }
}

void verify(HINTERNET request, std::string_view hostname,
            const settings& value)
{
    if (value.anchors.data().values.empty())
    {
        return;
    }
    (void)request;
    (void)hostname;
    network::fail("security_error",
        "自定义 CA 验证不得延迟到 HTTP 响应阶段");
}

} // namespace tx_generated::httpx_client_tls
