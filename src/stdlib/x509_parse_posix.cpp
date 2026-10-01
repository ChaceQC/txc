#include "stdlib/x509_openssl.hpp"

#include "stdlib/encoding.hpp"
#include "stdlib/error.hpp"

#include <openssl/evp.h>
#include <algorithm>

namespace tx_generated::x509
{
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
        if (count == 0 || count > 4 || data->size() < count + 2 || (*data)[2] == 0)
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
        else if ((!letter && !digit && character != '+' && character != '/') || padding)
        {
            fail("invalid_certificate", "PEM 证书含无效 Base64 字符");
        }
        encoded.push_back(character);
    }
    const auto first_padding = encoded.find('=');
    if (encoded.empty() || encoded.size() % 4 != 0 ||
        encoded.size() > max_certificate_bytes * 2 ||
        (first_padding != std::string::npos && first_padding < encoded.size() - 2))
    {
        fail("invalid_certificate", "PEM 证书 Base64 长度或填充无效");
    }
    std::vector<std::uint8_t> decoded(encoded.size() / 4 * 3);
    const int length = EVP_DecodeBlock(decoded.data(),
        reinterpret_cast<const unsigned char*>(encoded.data()), static_cast<int>(encoded.size()));
    if (length < 0)
    {
        fail("invalid_certificate", "PEM 证书 Base64 解码失败");
    }
    decoded.resize(static_cast<std::size_t>(length) -
        (first_padding == std::string::npos ? 0 : encoded.size() - first_padding));
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

void validate_text(std::string_view text)
{
    try
    {
        detail::validate_utf8(text);
    }
    catch (const runtime_failure&)
    {
        fail("invalid_argument", "证书参数不是有效 UTF-8");
    }
}

cert_ptr certificate(const byte_value& data)
{
    if (!data || data->empty() || data->size() > max_certificate_bytes)
    {
        fail(data && data->size() > max_certificate_bytes ? "size_limit" : "invalid_certificate",
            "单张证书必须在 1 字节到 1 MiB 之间");
    }
    check_der_length(data);
    const auto* cursor = data->data();
    cert_ptr result(d2i_X509(nullptr, &cursor, static_cast<long>(data->size())), X509_free);
    if (!result || cursor != data->data() + data->size())
    {
        fail("invalid_certificate", "证书 DER 解析失败或含尾部数据");
    }
    return result;
}

byte_value certificate_bytes(X509* value)
{
    const int length = i2d_X509(value, nullptr);
    if (length <= 0 || static_cast<std::size_t>(length) > max_certificate_bytes)
    {
        fail("size_limit", "证书大小超出限制");
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    auto* cursor = bytes.data();
    if (i2d_X509(value, &cursor) != length)
    {
        fail("operation_failed", "证书 DER 导出失败");
    }
    return make_bytes(std::move(bytes));
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
        auto value = decode_pem_body(text.substr(cursor, end - cursor));
        (void)certificate(value);
        result.push_back(std::move(value));
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
    // bytes 是不可变值；弱引用只记住本线程最近一次成功的结构解析，
    // 不保活证书，也不缓存任何依赖时间、信任或外部状态的验证结论。
    thread_local std::weak_ptr<const byte_storage> last_valid;
    if (const auto previous = last_valid.lock(); previous && previous == data)
    {
        return data;
    }
    (void)certificate(data);
    last_valid = data;
    return data;
}

} // namespace tx_generated::x509
