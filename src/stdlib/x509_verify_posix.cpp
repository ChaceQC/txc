#include "stdlib/x509_openssl.hpp"

#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
#include <arpa/inet.h>
#include <array>

namespace tx_generated::x509
{
namespace
{

int certificate_purpose(std::string_view purpose)
{
    if (purpose == "server_auth")
    {
        return X509_PURPOSE_SSL_SERVER;
    }
    if (purpose == "client_auth")
    {
        return X509_PURPOSE_SSL_CLIENT;
    }
    if (purpose == "code_signing")
    {
        return X509_PURPOSE_ANY;
    }
    fail("invalid_argument", "证书用途仅支持 server_auth/client_auth/code_signing");
}

const char* verification_status(int error)
{
    switch (error)
    {
    case X509_V_OK:
        return "valid";
    case X509_V_ERR_CERT_NOT_YET_VALID:
        return "not_yet_valid";
    case X509_V_ERR_CERT_HAS_EXPIRED:
        return "expired";
    case X509_V_ERR_HOSTNAME_MISMATCH:
    case X509_V_ERR_IP_ADDRESS_MISMATCH:
        return "hostname_mismatch";
    case X509_V_ERR_INVALID_PURPOSE:
        return "wrong_purpose";
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
    case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
    case X509_V_ERR_CERT_UNTRUSTED:
        return "unknown_issuer";
    default:
        return "invalid_chain";
    }
}

void set_hostname(X509_VERIFY_PARAM* parameters, std::string_view hostname)
{
    if (hostname.empty() || hostname.size() > 1024 ||
        hostname.find('\0') != std::string_view::npos)
    {
        fail("invalid_argument", "主机名为空、过长或含空字符");
    }
    validate_text(hostname);
    const std::string name(hostname);
    std::array<unsigned char, 16> address{};
    const bool ip = inet_pton(AF_INET, name.c_str(), address.data()) == 1 ||
        inet_pton(AF_INET6, name.c_str(), address.data()) == 1;
    X509_VERIFY_PARAM_set_hostflags(parameters, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
    const int result = ip ? X509_VERIFY_PARAM_set1_ip_asc(parameters, name.c_str()) :
        X509_VERIFY_PARAM_set1_host(parameters, name.c_str(), name.size());
    if (result != 1)
    {
        fail("invalid_argument", "证书验证主机名无效");
    }
}

bool permits_code_signing(X509* item)
{
    int critical = -1;
    using usage_ptr = std::unique_ptr<EXTENDED_KEY_USAGE, decltype(&EXTENDED_KEY_USAGE_free)>;
    usage_ptr usage(static_cast<EXTENDED_KEY_USAGE*>(X509_get_ext_d2i(item,
        NID_ext_key_usage, &critical, nullptr)), EXTENDED_KEY_USAGE_free);
    if (!usage)
    {
        return critical == -1;
    }
    for (int index = 0; index < sk_ASN1_OBJECT_num(usage.get()); ++index)
    {
        const auto nid = OBJ_obj2nid(sk_ASN1_OBJECT_value(usage.get(), index));
        if (nid == NID_code_sign || nid == NID_anyExtendedKeyUsage)
        {
            return true;
        }
    }
    return false;
}

} // namespace

verification verify(const byte_value& leaf, const bytes_vector& intermediates,
    const bytes_vector& trust_anchors, std::string_view hostname,
    std::string_view purpose, bool system_trust)
{
    const int usage = certificate_purpose(purpose);
    if (purpose != "server_auth" && !hostname.empty())
    {
        fail("invalid_argument", "非服务器证书验证不能提供主机名");
    }
    if (intermediates.data().values.size() > max_certificate_count ||
        trust_anchors.data().values.size() > max_certificate_count)
    {
        fail("size_limit", "中间证书或信任锚超过 64 张");
    }
    auto parsed_leaf = certificate(leaf);
    certificates_ptr additional(sk_X509_new_null());
    std::unique_ptr<X509_STORE, decltype(&X509_STORE_free)> store(X509_STORE_new(), X509_STORE_free);
    std::unique_ptr<X509_STORE_CTX, decltype(&X509_STORE_CTX_free)> context(
        X509_STORE_CTX_new(), X509_STORE_CTX_free);
    if (!additional || !store || !context)
    {
        fail("operation_failed", "创建证书链验证上下文失败");
    }
    for (const auto& value : intermediates.data().values)
    {
        auto item = certificate(value);
        if (!sk_X509_push(additional.get(), item.get()))
        {
            fail("operation_failed", "添加中间证书失败");
        }
        item.release();
    }
    for (const auto& value : trust_anchors.data().values)
    {
        const auto item = certificate(value);
        if (X509_STORE_add_cert(store.get(), item.get()) != 1)
        {
            fail("operation_failed", "添加信任锚失败");
        }
    }
    // 系统信任每次重新读取；不会发起 AIA、CRL 或 OCSP 网络查询。
    if ((system_trust && X509_STORE_set_default_paths(store.get()) != 1) ||
        X509_STORE_CTX_init(context.get(), store.get(), parsed_leaf.get(), additional.get()) != 1)
    {
        fail("operation_failed", "加载系统根证书或初始化证书链失败");
    }
    auto* parameters = X509_STORE_CTX_get0_param(context.get());
    X509_VERIFY_PARAM_set_depth(parameters, static_cast<int>(max_certificate_count) - 1);
    X509_VERIFY_PARAM_set_flags(parameters, X509_V_FLAG_TRUSTED_FIRST | X509_V_FLAG_PARTIAL_CHAIN);
    if (X509_VERIFY_PARAM_set_purpose(parameters, usage) != 1)
    {
        fail("operation_failed", "设置证书用途失败");
    }
    if (purpose == "server_auth")
    {
        set_hostname(parameters, hostname);
    }
    const int verified = X509_verify_cert(context.get());
    if (verified < 0)
    {
        fail("operation_failed", "执行证书链验证失败");
    }
    verification result;
    result.status = verification_status(X509_STORE_CTX_get_error(context.get()));
    const auto* chain = X509_STORE_CTX_get0_chain(context.get());
    for (int index = 0; chain && index < sk_X509_num(chain); ++index)
    {
        if (result.chain.size() >= max_certificate_count)
        {
            fail("size_limit", "构建的证书链超过 64 张");
        }
        auto* item = sk_X509_value(chain, index);
        if (result.status == "valid" && purpose == "code_signing" && !permits_code_signing(item))
        {
            result.status = "wrong_purpose";
        }
        result.chain.push_back(certificate_bytes(item));
    }
    return result;
}

} // namespace tx_generated::x509
