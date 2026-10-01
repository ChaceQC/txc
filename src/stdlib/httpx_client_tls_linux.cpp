#include "stdlib/httpx_client_tls.hpp"

#include "stdlib/error.hpp"
#include "stdlib/network_common.hpp"
#include "stdlib/tls.hpp"
#include "stdlib/tls_stream.hpp"
#include "stdlib/x509.hpp"

#include <openssl/ssl.h>
#include <openssl/x509.h>

namespace tx_generated::httpx_client_tls
{
struct settings
{
    bytes_vector anchors;
    bool include_system = true;
    std::int64_t identity_id = 0;
    ~settings()
    {
        tls::close_identity(identity_id);
    }
};

namespace
{
using certificate = std::unique_ptr<X509, decltype(&X509_free)>;

certificate decode(const byte_value& data)
{
    const auto* cursor = data->data();
    certificate value(d2i_X509(nullptr, &cursor, static_cast<long>(data->size())), X509_free);
    if (!value || cursor != data->data() + data->size())
    {
        network::fail("security_error", "客户端信任证书无效");
    }
    return value;
}

CURLcode configure_context(CURL*, void* context, void* user) noexcept
{
    try
    {
        auto* ssl = static_cast<SSL_CTX*>(context);
        const auto& value = *static_cast<const settings*>(user);
        if (!value.include_system)
        {
            auto* store = X509_STORE_new();
            if (!store)
            {
                return CURLE_OUT_OF_MEMORY;
            }
            SSL_CTX_set_cert_store(ssl, store);
        }
        for (const auto& anchor : value.anchors.data().values)
        {
            const auto cert = decode(anchor);
            if (X509_STORE_add_cert(SSL_CTX_get_cert_store(ssl), cert.get()) != 1)
            {
                return CURLE_SSL_CERTPROBLEM;
            }
        }
        if (value.identity_id != 0)
        {
            const auto identity = tls::get_identity(value.identity_id);
            std::lock_guard lock(identity->mutex);
            const auto leaf = decode(identity->certificates.front());
            if (SSL_CTX_use_certificate(ssl, leaf.get()) != 1)
            {
                return CURLE_SSL_CERTPROBLEM;
            }
            for (std::size_t index = 1; index < identity->certificates.size(); ++index)
            {
                const auto intermediate = decode(identity->certificates[index]);
                if (SSL_CTX_add1_chain_cert(ssl, intermediate.get()) != 1)
                {
                    return CURLE_SSL_CERTPROBLEM;
                }
            }
            const auto bytes = identity->private_key->view();
            const auto* cursor = bytes.data();
            std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(
                d2i_AutoPrivateKey(nullptr, &cursor, static_cast<long>(bytes.size())), EVP_PKEY_free);
            if (!key || cursor != bytes.data() + bytes.size() ||
                SSL_CTX_use_PrivateKey(ssl, key.get()) != 1 ||
                SSL_CTX_check_private_key(ssl) != 1)
            {
                return CURLE_SSL_CERTPROBLEM;
            }
        }
        return CURLE_OK;
    }
    catch (...)
    {
        return CURLE_SSL_CERTPROBLEM;
    }
}
}

std::shared_ptr<const settings> create(const bytes_vector& anchors,
    bool include_system, const byte_value& package, const secret::handle& password)
{
    const auto& roots = anchors.data().values;
    if ((!include_system && roots.empty()) || roots.size() > 64)
    {
        network::fail("invalid_argument", "HTTP 信任锚配置无效");
    }
    auto result = std::make_shared<settings>();
    result->anchors = anchors.copy();
    result->include_system = include_system;
    for (const auto& root : roots)
    {
        (void)x509::parse_der(root);
    }
    if (!package->empty())
    {
        if (package->size() > 16 * 1024 * 1024)
        {
            network::fail("size_limit", "客户端证书包超过 16 MiB");
        }
        result->identity_id = tls::import_identity(package, password);
    }
    return result;
}

bool has_custom_anchors(const settings& value) noexcept
{
    return !value.anchors.data().values.empty();
}

void configure(CURL* request, const settings& value)
{
    const auto* backend = curl_version_info(CURLVERSION_NOW)->ssl_version;
    if (!backend || std::string_view(backend).find("OpenSSL") == std::string_view::npos)
    {
        network::fail("security_error", "Linux HTTPS 需要使用 OpenSSL 后端的 libcurl");
    }
    if (curl_easy_setopt(request, CURLOPT_SSL_CTX_FUNCTION, configure_context) != CURLE_OK ||
        curl_easy_setopt(request, CURLOPT_SSL_CTX_DATA, &value) != CURLE_OK)
    {
        network::fail("security_error", "配置 HTTPS 握手验证失败");
    }
}

std::shared_ptr<tls::secure_connection> connect_custom(
    const settings& value, std::shared_ptr<network::socket_handle> socket,
    std::string_view hostname, const std::vector<std::string>& protocols,
    std::int64_t timeout_ms, bool allow_no_alpn)
{
    tls::client_options options;
    options.hostname = hostname;
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
            network::fail(failure.error().code == "alpn_mismatch"
                ? "protocol_error" : "security_error", failure.error().message);
        }
        throw;
    }
}
}
