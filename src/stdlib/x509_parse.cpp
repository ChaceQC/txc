#include "stdlib/x509_win.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace tx_generated::x509
{

#ifdef _WIN32
namespace
{

constexpr std::string_view begin_marker = "-----BEGIN CERTIFICATE-----";
constexpr std::string_view end_marker = "-----END CERTIFICATE-----";

bool whitespace(char value) noexcept
{
    return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

void check_der_length(const byte_value& data)
{
    if (data->size() < 2 || data->front() != 0x30)
    {
        fail("invalid_certificate", "证书不是完整的 DER SEQUENCE");
    }
    const auto first = (*data)[1];
    std::size_t content_size = first;
    std::size_t header_size = 2;
    if ((first & 0x80) != 0)
    {
        const auto count = static_cast<std::size_t>(first & 0x7f);
        if (count == 0 || count > 4 || data->size() < count + 2 ||
            (*data)[2] == 0)
        {
            fail("invalid_certificate", "证书 DER 长度无效");
        }
        content_size = 0;
        for (std::size_t index = 0; index < count; ++index)
        {
            content_size = (content_size << 8) | (*data)[index + 2];
        }
        if (content_size < 128)
        {
            fail("invalid_certificate", "证书 DER 长度不是最短编码");
        }
        header_size += count;
    }
    if (content_size != data->size() - header_size)
    {
        fail("invalid_certificate", "证书 DER 含截断或尾部数据");
    }
}

byte_value decode_pem_body(std::string_view body)
{
    std::string encoded;
    encoded.reserve(body.size());
    bool padding = false;
    for (const char character : body)
    {
        if (whitespace(character))
        {
            continue;
        }
        const bool letter = (character >= 'A' && character <= 'Z') ||
            (character >= 'a' && character <= 'z');
        const bool digit = character >= '0' && character <= '9';
        if (character == '=')
        {
            padding = true;
        }
        else if ((!letter && !digit && character != '+' && character != '/') ||
                 padding)
        {
            fail("invalid_certificate", "PEM 证书含无效 Base64 字符");
        }
        encoded.push_back(character);
    }
    if (encoded.empty() || encoded.size() % 4 != 0 ||
        encoded.size() > max_certificate_bytes * 2)
    {
        fail("invalid_certificate", "PEM 证书 Base64 长度无效");
    }
    const auto first_padding = encoded.find('=');
    if (first_padding != std::string::npos &&
        (first_padding < encoded.size() - 2 ||
         encoded.size() - first_padding > 2))
    {
        fail("invalid_certificate", "PEM 证书 Base64 填充无效");
    }
    DWORD decoded_size = 0;
    if (!CryptStringToBinaryA(encoded.data(),
            static_cast<DWORD>(encoded.size()), CRYPT_STRING_BASE64,
            nullptr, &decoded_size, nullptr, nullptr) ||
        decoded_size > max_certificate_bytes)
    {
        fail("invalid_certificate", "PEM 证书 Base64 解码失败");
    }
    std::vector<std::uint8_t> decoded(decoded_size);
    if (!CryptStringToBinaryA(encoded.data(),
            static_cast<DWORD>(encoded.size()), CRYPT_STRING_BASE64,
            decoded.data(), &decoded_size, nullptr, nullptr))
    {
        fail("invalid_certificate", "PEM 证书 Base64 解码失败");
    }
    decoded.resize(decoded_size);
    return make_bytes(std::move(decoded));
}

} // namespace

[[noreturn]] void fail(const char* code, const char* message)
{
    throw runtime_failure({tx::error_kind::security, code, message});
}

void check_input_size(std::size_t size)
{
    if (size == 0 || size > max_input_bytes)
    {
        fail(size == 0 ? "invalid_certificate" : "size_limit",
            size == 0 ? "证书输入为空" : "证书输入超过 16 MiB 限制");
    }
}

cert_ptr certificate(const byte_value& data)
{
    if (data->empty() || data->size() > max_certificate_bytes)
    {
        fail(data->empty() ? "invalid_certificate" : "size_limit",
            "单张证书必须在 1 字节到 1 MiB 之间");
    }
    check_der_length(data);
    cert_ptr result(CertCreateCertificateContext(X509_ASN_ENCODING,
        data->data(), static_cast<DWORD>(data->size())),
        &CertFreeCertificateContext);
    if (!result)
    {
        fail("invalid_certificate", "证书 DER 解析失败");
    }
    return result;
}

byte_value certificate_bytes(PCCERT_CONTEXT value)
{
    if (!value || value->cbCertEncoded == 0 ||
        value->cbCertEncoded > max_certificate_bytes)
    {
        fail("size_limit", "证书大小超出限制");
    }
    return make_bytes({value->pbCertEncoded,
        value->pbCertEncoded + value->cbCertEncoded});
}

store_ptr memory_store()
{
    store_ptr result(CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
        CERT_STORE_CREATE_NEW_FLAG, nullptr));
    if (!result)
    {
        fail("operation_failed", "创建证书存储失败");
    }
    return result;
}

std::wstring utf8_wide(std::string_view text)
{
    if (text.empty() || text.size() > 1024 ||
        text.find('\0') != std::string_view::npos)
    {
        fail("invalid_argument", "主机名为空、过长或含空字符");
    }
    const auto length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0)
    {
        fail("invalid_argument", "主机名不是有效 UTF-8");
    }
    std::wstring result(length, L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            text.data(), static_cast<int>(text.size()), result.data(), length) != length)
    {
        fail("invalid_argument", "主机名编码失败");
    }
    return result;
}

std::vector<byte_value> parse_pem(std::string_view text)
{
    check_input_size(text.size());
    std::vector<byte_value> result;
    std::size_t cursor = 0;
    while (cursor < text.size())
    {
        while (cursor < text.size() && whitespace(text[cursor]))
        {
            ++cursor;
        }
        if (cursor == text.size())
        {
            break;
        }
        if (!text.substr(cursor).starts_with(begin_marker))
        {
            fail("invalid_certificate", "PEM 证书块开头无效");
        }
        cursor += begin_marker.size();
        const auto end = text.find(end_marker, cursor);
        if (end == std::string_view::npos)
        {
            fail("invalid_certificate", "PEM 证书块缺少结束标记");
        }
        const auto value = decode_pem_body(text.substr(cursor, end - cursor));
        (void)certificate(value);
        result.push_back(value);
        if (result.size() > max_certificate_count)
        {
            fail("size_limit", "证书数量超过 64 张");
        }
        cursor = end + end_marker.size();
    }
    if (result.empty())
    {
        fail("invalid_certificate", "PEM 输入没有证书");
    }
    return result;
}

byte_value parse_der(const byte_value& data)
{
    (void)certificate(data);
    return data;
}

#else

std::vector<byte_value> parse_pem(std::string_view)
{
    throw runtime_failure({tx::error_kind::security, "unsupported_platform",
        "此平台尚不支持 X.509 证书读取"});
}

byte_value parse_der(const byte_value&)
{
    throw runtime_failure({tx::error_kind::security, "unsupported_platform",
        "此平台尚不支持 X.509 证书读取"});
}

#endif

} // namespace tx_generated::x509
