#include "stdlib/x509_win.hpp"
#include "stdlib/x509_pkcs8.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#ifdef _WIN32
#include <ncrypt.h>
#endif

namespace tx_generated::x509
{

#ifdef _WIN32
namespace
{

std::size_t password_character_count(std::span<const std::uint8_t> bytes)
{
    if (bytes.size() > 4096 ||
        std::find(bytes.begin(), bytes.end(), 0) != bytes.end())
    {
        fail("invalid_argument", "PKCS#12 密码长度无效或含空字符");
    }
    if (bytes.empty())
    {
        return 0;
    }
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<int>(bytes.size()), nullptr, 0);
    if (count <= 0)
    {
        fail("invalid_argument", "PKCS#12 密码不是有效 UTF-8");
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
    explicit password_buffer(const secret::handle& password)
        : value_(password_character_count(password->view()))
    {
        const auto bytes = password->view();
        if (bytes.empty())
        {
            return;
        }
        const auto characters = value_.characters();
        const int converted = MultiByteToWideChar(CP_UTF8,
            MB_ERR_INVALID_CHARS,
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<int>(bytes.size()), value_.data(),
            static_cast<int>(characters));
        if (converted <= 0 ||
            static_cast<std::size_t>(converted) != characters)
        {
            fail("operation_failed", "PKCS#12 密码编码失败");
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

store_ptr import_store(const byte_value& data, const secret::handle& password)
{
    if (data->empty())
    {
        fail("invalid_pkcs12", "PKCS#12 输入为空");
    }
    check_input_size(data->size());
    CRYPT_DATA_BLOB package{static_cast<DWORD>(data->size()),
        const_cast<BYTE*>(data->data())};
    if (!PFXIsPFXBlob(&package))
    {
        fail("invalid_pkcs12", "输入不是 PKCS#12 数据包");
    }
    const password_buffer converted(password);
    constexpr DWORD flags = CRYPT_EXPORTABLE | PKCS12_ALWAYS_CNG_KSP |
        PKCS12_NO_PERSIST_KEY | CRYPT_USER_KEYSET;
    store_ptr result(PFXImportCertStore(&package, converted.data(), flags));
    if (!result)
    {
        fail("invalid_password_or_data", "PKCS#12 密码或数据无效");
    }
    return result;
}

std::optional<CERT_KEY_CONTEXT> private_key_context(PCCERT_CONTEXT certificate)
{
    // NO_PERSIST_KEY 把密钥句柄挂在证书上下文，而不是持久化提供者属性。
    CERT_KEY_CONTEXT context{};
    DWORD size = sizeof(context);
    if (!CertGetCertificateContextProperty(certificate,
            CERT_KEY_CONTEXT_PROP_ID, &context, &size))
    {
        if (GetLastError() != static_cast<DWORD>(CRYPT_E_NOT_FOUND))
        {
            fail("operation_failed", "读取 PKCS#12 密钥关联失败");
        }
        return std::nullopt;
    }
    if (size < sizeof(context) || context.cbSize < sizeof(context))
    {
        fail("operation_failed", "PKCS#12 密钥关联数据无效");
    }
    return context;
}

} // namespace

std::vector<byte_value> parse_pkcs12(const byte_value& data,
                                     const secret::handle& password)
{
    auto store = import_store(data, password);
    std::vector<byte_value> result;
    byte_value owner;
    std::size_t count = 0;
    certificate_cursor cursor;
    while (const auto* current = cursor.next(store.get()))
    {
        if (++count > max_certificate_count)
        {
            fail("size_limit", "PKCS#12 证书数量超过 64 张");
        }
        auto encoded = certificate_bytes(current);
        if (private_key_context(current))
        {
            if (owner)
            {
                fail("invalid_pkcs12", "PKCS#12 含多个私钥身份");
            }
            owner = std::move(encoded);
        }
        else
        {
            result.push_back(std::move(encoded));
        }
    }
    if (owner)
    {
        result.insert(result.begin(), std::move(owner));
    }
    if (result.empty())
    {
        fail("invalid_pkcs12", "PKCS#12 数据包没有证书");
    }
    return result;
}

secret::handle pkcs12_private_key(const byte_value& data,
                                  const secret::handle& password)
{
    auto store = import_store(data, password);
    secret::handle result;
    std::size_t count = 0;
    certificate_cursor cursor;
    while (const auto* current = cursor.next(store.get()))
    {
        if (++count > max_certificate_count)
        {
            fail("size_limit", "PKCS#12 证书数量超过 64 张");
        }
        const auto key = private_key_context(current);
        if (!key)
        {
            continue;
        }
        if (result)
        {
            fail("invalid_pkcs12", "PKCS#12 含多个私钥身份");
        }
        if (key->dwKeySpec != CERT_NCRYPT_KEY_SPEC)
        {
            fail("unsupported_key", "PKCS#12 私钥无法以 PKCS#8 导出");
        }
        result = export_private_key(key->hNCryptKey);
    }
    if (!result)
    {
        fail("no_private_key", "PKCS#12 数据包没有私钥");
    }
    return result;
}

#else

std::vector<byte_value> parse_pkcs12(const byte_value&,
                                     const secret::handle&)
{
    throw runtime_failure({tx::error_kind::security, "unsupported_platform",
        "此平台尚不支持 PKCS#12 读取"});
}

secret::handle pkcs12_private_key(const byte_value&,
                                  const secret::handle&)
{
    throw runtime_failure({tx::error_kind::security, "unsupported_platform",
        "此平台尚不支持 PKCS#12 私钥读取"});
}

#endif

} // namespace tx_generated::x509
