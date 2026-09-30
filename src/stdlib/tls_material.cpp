#include "stdlib/tls_material.hpp"
#include "stdlib/crypto_internal.hpp"
#include "stdlib/error.hpp"

#include <mbedtls/entropy.h>

namespace tx_generated::tls
{
namespace
{

void require_material(int status, const char* action)
{
    if (status != 0)
    {
        throw runtime_failure({tx::error_kind::security, "handshake_failed",
            std::string(action) + "失败，TLS 错误码 " + std::to_string(status)});
    }
}

} // namespace

identity_material::identity_material()
{
    mbedtls_x509_crt_init(&certificates);
    mbedtls_pk_init(&key);
}

identity_material::~identity_material()
{
    mbedtls_pk_free(&key);
    mbedtls_x509_crt_free(&certificates);
}

int tls_random(void*, unsigned char* output, std::size_t size) noexcept
{
    try
    {
        // 复用已有启用线程互斥的 PSA CSPRNG；每次仍生成新随机数，绝不缓存随机输出。
        crypto::fill_random({output, size});
        return 0;
    }
    catch (...)
    {
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }
}

std::shared_ptr<identity_material> acquire_identity_material(const std::shared_ptr<const identity_state>& identity)
{
    std::lock_guard lock(identity->mutex);
    if (identity->closed)
    {
        throw runtime_failure({tx::error_kind::security, "invalid_state", "TLS 身份不存在或已关闭"});
    }
    if (!identity->available_materials.empty())
    {
        auto material = std::move(identity->available_materials.back());
        identity->available_materials.pop_back();
        return material;
    }
    if (identity->certificates.empty())
    {
        throw runtime_failure({tx::error_kind::security, "invalid_certificate", "TLS 身份没有叶证书"});
    }
    auto material = std::make_shared<identity_material>();
    for (const auto& certificate : identity->certificates)
    {
        require_material(mbedtls_x509_crt_parse_der(&material->certificates,
            certificate->data(), certificate->size()), "加载 TLS 身份证书");
    }
    const auto key = identity->private_key->view();
    require_material(mbedtls_pk_parse_key(&material->key, key.data(), key.size(), nullptr, 0,
        tls_random, nullptr), "加载 TLS 私钥");
    return material;
}

void release_identity_material(const std::shared_ptr<const identity_state>& identity,
    std::shared_ptr<identity_material> material) noexcept
{
    if (!identity || !material)
    {
        return;
    }
    try
    {
        std::lock_guard lock(identity->mutex);
        if (!identity->closed && identity->available_materials.size() < 4)
        {
            // 私钥操作可能修改内部盲化状态，一份 material 只能租给一个活动连接。
            identity->available_materials.push_back(std::move(material));
        }
    }
    catch (...)
    {
        // 归还分配失败时直接销毁并清零解析后的私钥。
    }
}

} // namespace tx_generated::tls
