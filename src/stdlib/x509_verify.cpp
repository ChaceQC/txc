#include "stdlib/x509_win.hpp"

#include "stdlib/error.hpp"

#include <string>

namespace tx_generated::x509
{

#ifdef _WIN32
namespace
{

void add_certificate(HCERTSTORE store, const byte_value& data)
{
    auto parsed = certificate(data);
    if (!CertAddCertificateContextToStore(store, parsed.get(),
            CERT_STORE_ADD_USE_EXISTING, nullptr))
    {
        fail("operation_failed", "添加证书到验证存储失败");
    }
}

void add_system_roots(HCERTSTORE roots, DWORD location)
{
    store_ptr system(CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
        location | CERT_STORE_READONLY_FLAG, L"ROOT"));
    if (!system)
    {
        fail("operation_failed", "读取系统根证书存储失败");
    }
    certificate_cursor cursor;
    while (const auto* current = cursor.next(system.get()))
    {
        if (!CertAddCertificateContextToStore(roots, current,
                CERT_STORE_ADD_USE_EXISTING, nullptr))
        {
            fail("operation_failed", "加入系统信任锚失败");
        }
    }
}

const char* usage_oid(std::string_view purpose)
{
    if (purpose == "server_auth")
    {
        return szOID_PKIX_KP_SERVER_AUTH;
    }
    if (purpose == "client_auth")
    {
        return szOID_PKIX_KP_CLIENT_AUTH;
    }
    if (purpose == "code_signing")
    {
        return szOID_PKIX_KP_CODE_SIGNING;
    }
    fail("invalid_argument", "证书用途仅支持 server_auth/client_auth/code_signing");
}

std::string time_status(PCCERT_CHAIN_CONTEXT chain)
{
    if (chain->cChain == 0)
    {
        return {};
    }
    const auto* simple = chain->rgpChain[0];
    for (DWORD index = 0; index < simple->cElement; ++index)
    {
        auto* information = simple->rgpElement[index]->pCertContext->pCertInfo;
        const auto relation = CertVerifyTimeValidity(nullptr, information);
        if (relation < 0)
        {
            return "not_yet_valid";
        }
        if (relation > 0)
        {
            return "expired";
        }
    }
    return {};
}

DWORD policy_error(PCCERT_CHAIN_CONTEXT chain, std::string_view purpose,
                   std::string_view hostname)
{
    CERT_CHAIN_POLICY_PARA parameters{};
    parameters.cbSize = sizeof(parameters);
    SSL_EXTRA_CERT_CHAIN_POLICY_PARA ssl{};
    std::wstring server_name;
    LPCSTR policy = CERT_CHAIN_POLICY_BASE;
    if (purpose == "server_auth")
    {
        server_name = utf8_wide(hostname);
        ssl.cbSize = sizeof(ssl);
        ssl.dwAuthType = AUTHTYPE_SERVER;
        ssl.pwszServerName = server_name.data();
        parameters.pvExtraPolicyPara = &ssl;
        policy = CERT_CHAIN_POLICY_SSL;
    }
    CERT_CHAIN_POLICY_STATUS status{};
    status.cbSize = sizeof(status);
    if (!CertVerifyCertificateChainPolicy(policy, chain, &parameters, &status))
    {
        fail("operation_failed", "执行证书链策略验证失败");
    }
    return status.dwError;
}

std::string verification_status(PCCERT_CHAIN_CONTEXT chain, DWORD policy)
{
    const auto errors = chain->TrustStatus.dwErrorStatus;
    if ((errors & (CERT_TRUST_IS_NOT_SIGNATURE_VALID |
                   CERT_TRUST_INVALID_BASIC_CONSTRAINTS |
                   CERT_TRUST_IS_CYCLIC)) != 0)
    {
        return "invalid_chain";
    }
    if (const auto time = time_status(chain); !time.empty())
    {
        return time;
    }
    if (policy == static_cast<DWORD>(CERT_E_CN_NO_MATCH))
    {
        return "hostname_mismatch";
    }
    if ((errors & CERT_TRUST_IS_NOT_VALID_FOR_USAGE) != 0 ||
        policy == static_cast<DWORD>(CERT_E_WRONG_USAGE))
    {
        return "wrong_purpose";
    }
    if ((errors & (CERT_TRUST_IS_UNTRUSTED_ROOT |
                   CERT_TRUST_IS_PARTIAL_CHAIN)) != 0 ||
        policy == static_cast<DWORD>(CERT_E_UNTRUSTEDROOT) ||
        policy == static_cast<DWORD>(CERT_E_CHAINING) ||
        policy == static_cast<DWORD>(CERT_E_UNTRUSTEDCA))
    {
        return "unknown_issuer";
    }
    return errors == 0 && policy == ERROR_SUCCESS ? "valid" : "invalid_chain";
}

} // namespace

verification verify(const byte_value& leaf, const bytes_vector& intermediates,
                    const bytes_vector& trust_anchors, std::string_view hostname,
                    std::string_view purpose, bool system_trust)
{
    const auto* oid = usage_oid(purpose);
    if (purpose == "server_auth")
    {
        (void)utf8_wide(hostname);
    }
    else if (!hostname.empty())
    {
        fail("invalid_argument", "非服务器证书验证不能提供主机名");
    }
    if (intermediates.data().values.size() > max_certificate_count ||
        trust_anchors.data().values.size() > max_certificate_count)
    {
        fail("size_limit", "中间证书或信任锚超过 64 张");
    }

    auto leaf_context = certificate(leaf);
    auto additional = memory_store();
    for (const auto& item : intermediates.data().values)
    {
        add_certificate(additional.get(), item);
    }
    auto roots = memory_store();
    for (const auto& item : trust_anchors.data().values)
    {
        add_certificate(roots.get(), item);
    }
    if (system_trust)
    {
        add_system_roots(roots.get(), CERT_SYSTEM_STORE_CURRENT_USER);
        add_system_roots(roots.get(), CERT_SYSTEM_STORE_LOCAL_MACHINE);
    }

    CERT_CHAIN_ENGINE_CONFIG engine_config{};
    engine_config.cbSize = sizeof(engine_config);
    // 只把显式加入的根视为锚；中间证书存储不授予信任。
    engine_config.hExclusiveRoot = roots.get();
    HCERTCHAINENGINE raw_engine = nullptr;
    if (!CertCreateCertificateChainEngine(&engine_config, &raw_engine))
    {
        fail("operation_failed", "创建证书链引擎失败");
    }
    engine_ptr engine(raw_engine);

    char* requested_oid = const_cast<char*>(oid);
    CERT_CHAIN_PARA chain_parameters{};
    chain_parameters.cbSize = sizeof(chain_parameters);
    chain_parameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    chain_parameters.RequestedUsage.Usage.cUsageIdentifier = 1;
    chain_parameters.RequestedUsage.Usage.rgpszUsageIdentifier = &requested_oid;
    PCCERT_CHAIN_CONTEXT raw_chain = nullptr;
    constexpr DWORD flags = CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL |
        CERT_CHAIN_DISABLE_AUTH_ROOT_AUTO_UPDATE;
    if (!CertGetCertificateChain(engine.get(), leaf_context.get(), nullptr,
            additional.get(), &chain_parameters, flags, nullptr, &raw_chain))
    {
        fail("operation_failed", "构建证书链失败");
    }
    chain_ptr chain(raw_chain, &CertFreeCertificateChain);
    verification result;
    result.status = verification_status(chain.get(),
        policy_error(chain.get(), purpose, hostname));
    if (chain->cChain == 0)
    {
        fail("operation_failed", "证书链引擎没有返回链");
    }
    const auto* simple = chain->rgpChain[0];
    for (DWORD index = 0; index < simple->cElement; ++index)
    {
        if (result.chain.size() >= max_certificate_count)
        {
            fail("size_limit", "构建的证书链超过 64 张");
        }
        result.chain.push_back(certificate_bytes(
            simple->rgpElement[index]->pCertContext));
    }
    return result;
}

#else

verification verify(const byte_value&, const bytes_vector&, const bytes_vector&,
                    std::string_view, std::string_view, bool)
{
    throw runtime_failure({tx::error_kind::security, "unsupported_platform",
        "此平台尚不支持 X.509 证书链验证"});
}

#endif

} // namespace tx_generated::x509
