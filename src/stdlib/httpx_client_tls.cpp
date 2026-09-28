#include "stdlib/httpx_client_tls.hpp"

#include "stdlib/encoding.hpp"
#include "stdlib/httpx.hpp"
#include "stdlib/x509_win.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include <ncrypt.h>

namespace tx_generated::httpx_client_tls
{
namespace
{

class password_buffer
{
public:
    explicit password_buffer(const secret::handle& source)
    {
        const auto raw = source->view();
        if (raw.size() > 4096 || std::find(raw.begin(), raw.end(), 0) != raw.end())
        {
            network::fail("invalid_argument", "客户端证书密码长度无效");
        }
        const std::string text = raw.empty() ? std::string{} :
            std::string(reinterpret_cast<const char*>(raw.data()), raw.size());
        value_ = detail::utf8_to_wide(text);
        value_.push_back(L'\0');
    }

    ~password_buffer()
    {
        if (!value_.empty())
        {
            SecureZeroMemory(value_.data(), value_.size() * sizeof(wchar_t));
        }
    }

    password_buffer(const password_buffer&) = delete;
    password_buffer& operator=(const password_buffer&) = delete;

    const wchar_t* data() const noexcept
    {
        return value_.data();
    }

private:
    std::wstring value_;
};

byte_value certificate_bytes(PCCERT_CONTEXT value)
{
    if (!value || value->cbCertEncoded == 0 ||
        value->cbCertEncoded > x509::max_certificate_bytes)
    {
        network::fail("security_error", "HTTP 服务端证书长度无效");
    }
    return std::make_shared<const std::vector<std::uint8_t>>(
        value->pbCertEncoded, value->pbCertEncoded + value->cbCertEncoded);
}

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

    ~settings()
    {
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
    const password_buffer converted(password);
    constexpr DWORD flags = PKCS12_ALWAYS_CNG_KSP | CRYPT_USER_KEYSET;
    auto* store = PFXImportCertStore(&blob, converted.data(), flags);
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

void configure(HINTERNET request, const settings& value)
{
    if (!value.anchors.data().values.empty())
    {
        // WinHTTP 只放宽未知根；收到响应头前由独立证书链引擎完成严格校验。
        DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA;
        if (!WinHttpSetOption(request, WINHTTP_OPTION_SECURITY_FLAGS,
                              &flags, sizeof(flags)))
        {
            network::http_failure("配置自定义 HTTP 信任锚");
        }
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
    PCCERT_CHAIN_CONTEXT raw = nullptr;
    DWORD size = sizeof(raw);
    if (!WinHttpQueryOption(request, WINHTTP_OPTION_SERVER_CERT_CHAIN_CONTEXT,
                            &raw, &size) || !raw)
    {
        network::fail("security_error", "无法取得 HTTP 服务端证书链");
    }
    x509::chain_ptr chain(raw, &CertFreeCertificateChain);
    if (chain->cChain == 0 || chain->rgpChain[0]->cElement == 0 ||
        chain->rgpChain[0]->cElement > 64)
    {
        network::fail("security_error", "HTTP 服务端证书链无效");
    }
    const auto* simple = chain->rgpChain[0];
    bytes_vector intermediates;
    for (DWORD index = 1; index < simple->cElement; ++index)
    {
        intermediates.data().values.push_back(certificate_bytes(
            simple->rgpElement[index]->pCertContext));
    }
    intermediates.data().refresh();
    const auto leaf = certificate_bytes(simple->rgpElement[0]->pCertContext);
    const auto checked = x509::verify(leaf, intermediates, value.anchors,
                                     hostname, "server_auth", value.include_system);
    if (checked.status != "valid")
    {
        network::fail("security_error", "HTTP 服务端证书验证失败：" + checked.status);
    }
}

} // namespace tx_generated::httpx_client_tls
