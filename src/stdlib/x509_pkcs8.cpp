#include "stdlib/x509_pkcs8.hpp"

#ifdef _WIN32
#include "stdlib/x509_win.hpp"

namespace tx_generated::x509
{

secret::handle export_private_key(NCRYPT_KEY_HANDLE key)
{
    DWORD policy = 0;
    DWORD policy_size = 0;
    if (NCryptGetProperty(key, NCRYPT_EXPORT_POLICY_PROPERTY,
            reinterpret_cast<PBYTE>(&policy), sizeof(policy),
            &policy_size, 0) != ERROR_SUCCESS ||
        policy_size != sizeof(policy))
    {
        fail("unsupported_key", "读取 PKCS#12 私钥导出策略失败");
    }
    if ((policy & NCRYPT_ALLOW_PLAINTEXT_EXPORT_FLAG) == 0)
    {
        // 仅提升本次无持久化导入的临时句柄；明文只进入受控 secret_bytes。
        DWORD desired = policy | NCRYPT_ALLOW_PLAINTEXT_EXPORT_FLAG;
        if (NCryptSetProperty(key, NCRYPT_EXPORT_POLICY_PROPERTY,
                reinterpret_cast<PBYTE>(&desired), sizeof(desired), 0) !=
            ERROR_SUCCESS)
        {
            fail("unsupported_key", "PKCS#12 私钥不允许导出 PKCS#8");
        }
    }
    DWORD size = 0;
    if (NCryptExportKey(key, 0, NCRYPT_PKCS8_PRIVATE_KEY_BLOB,
            nullptr, nullptr, 0, &size, 0) != ERROR_SUCCESS ||
        size == 0 || size > max_certificate_bytes)
    {
        fail("unsupported_key", "PKCS#12 私钥算法或导出方式不受支持");
    }
    auto result = std::make_shared<secret::buffer>(size);
    DWORD written = 0;
    if (NCryptExportKey(key, 0, NCRYPT_PKCS8_PRIVATE_KEY_BLOB,
            nullptr, result->writable().data(), size, &written, 0) !=
        ERROR_SUCCESS || written != size)
    {
        fail("unsupported_key", "PKCS#12 私钥导出失败");
    }
    return result;
}

} // namespace tx_generated::x509
#endif
