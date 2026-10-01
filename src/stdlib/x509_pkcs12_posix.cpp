#include "stdlib/x509_openssl.hpp"

#include <openssl/pkcs12.h>
#include <algorithm>

namespace tx_generated::x509
{
namespace
{

using key_ptr = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;

void clear_private_info(const PKCS8_PRIV_KEY_INFO* value) noexcept
{
    const unsigned char* data = nullptr;
    int length = 0;
    if (value && PKCS8_pkey_get0(nullptr, &data, &length, nullptr, value) == 1 && length > 0)
    {
        OPENSSL_cleanse(const_cast<unsigned char*>(data), static_cast<std::size_t>(length));
    }
}

struct private_info_closer
{
    void operator()(PKCS8_PRIV_KEY_INFO* value) const noexcept
    {
        clear_private_info(value);
        PKCS8_PRIV_KEY_INFO_free(value);
    }
};

void clear_private_bags(const STACK_OF(PKCS12_SAFEBAG)* value, unsigned depth) noexcept
{
    if (!value || depth > 8)
    {
        return;
    }
    for (int index = 0; index < sk_PKCS12_SAFEBAG_num(value); ++index)
    {
        const auto* bag = sk_PKCS12_SAFEBAG_value(value, index);
        if (PKCS12_SAFEBAG_get_nid(bag) == NID_keyBag)
        {
            clear_private_info(PKCS12_SAFEBAG_get0_p8inf(bag));
        }
        else if (PKCS12_SAFEBAG_get_nid(bag) == NID_safeContentsBag)
        {
            clear_private_bags(PKCS12_SAFEBAG_get0_safes(bag), depth + 1);
        }
    }
}

struct bags_closer
{
    void operator()(STACK_OF(PKCS12_SAFEBAG)* value) const noexcept
    {
        clear_private_bags(value, 0);
        sk_PKCS12_SAFEBAG_pop_free(value, PKCS12_SAFEBAG_free);
    }
};

struct safes_closer
{
    void operator()(STACK_OF(PKCS7)* value) const noexcept
    {
        sk_PKCS7_pop_free(value, PKCS7_free);
    }
};

class password_text
{
public:
    explicit password_text(const secret::handle& password)
    {
        if (!password)
        {
            fail("invalid_argument", "PKCS#12 密码未提供");
        }
        const auto bytes = password->view();
        if (bytes.size() > 4096 || std::find(bytes.begin(), bytes.end(), 0) != bytes.end())
        {
            fail("invalid_argument", "PKCS#12 密码长度无效或含空字符");
        }
        validate_text({reinterpret_cast<const char*>(bytes.data()), bytes.size()});
        value_ = std::make_shared<secret::buffer>(bytes.size() + 1);
        std::copy(bytes.begin(), bytes.end(), value_->writable().begin());
        value_->writable().back() = 0;
    }

    const char* data() const
    {
        return reinterpret_cast<const char*>(value_->view().data());
    }

private:
    secret::handle value_;
};

struct imported_identity
{
    std::vector<cert_ptr> certificates;
    key_ptr key{nullptr, EVP_PKEY_free};
};

void read_bags(const STACK_OF(PKCS12_SAFEBAG)* bags, const char* password,
    imported_identity& identity, unsigned depth)
{
    if (depth > 8 || !bags)
    {
        fail("invalid_pkcs12", "PKCS#12 包结构无效或嵌套过深");
    }
    for (int index = 0; index < sk_PKCS12_SAFEBAG_num(bags); ++index)
    {
        const auto* bag = sk_PKCS12_SAFEBAG_value(bags, index);
        const int type = PKCS12_SAFEBAG_get_nid(bag);
        if (type == NID_safeContentsBag)
        {
            read_bags(PKCS12_SAFEBAG_get0_safes(bag), password, identity, depth + 1);
        }
        else if (type == NID_certBag && PKCS12_SAFEBAG_get_bag_nid(bag) == NID_x509Certificate)
        {
            cert_ptr item(PKCS12_SAFEBAG_get1_cert(bag), X509_free);
            if (!item)
            {
                fail("invalid_pkcs12", "PKCS#12 证书解析失败");
            }
            identity.certificates.push_back(std::move(item));
            if (identity.certificates.size() > max_certificate_count)
            {
                fail("size_limit", "PKCS#12 证书数量超过 64 张");
            }
        }
        else if (type == NID_keyBag || type == NID_pkcs8ShroudedKeyBag)
        {
            if (identity.key)
            {
                fail("invalid_pkcs12", "PKCS#12 含多个私钥身份");
            }
            std::unique_ptr<PKCS8_PRIV_KEY_INFO, private_info_closer> decrypted;
            const PKCS8_PRIV_KEY_INFO* info = nullptr;
            if (type == NID_pkcs8ShroudedKeyBag)
            {
                decrypted.reset(PKCS12_decrypt_skey(bag, password, -1));
                info = decrypted.get();
            }
            else
            {
                info = PKCS12_SAFEBAG_get0_p8inf(bag);
            }
            if (!info || !(identity.key = key_ptr(EVP_PKCS82PKEY(info), EVP_PKEY_free)))
            {
                fail("invalid_password_or_data", "PKCS#12 密码或私钥数据无效");
            }
        }
    }
}

imported_identity import_package(const byte_value& data, const secret::handle& password)
{
    if (!data || data->empty())
    {
        fail("invalid_pkcs12", "PKCS#12 输入为空");
    }
    check_input_size(data->size());
    const auto* cursor = data->data();
    std::unique_ptr<PKCS12, decltype(&PKCS12_free)> package(
        d2i_PKCS12(nullptr, &cursor, static_cast<long>(data->size())), PKCS12_free);
    if (!package || cursor != data->data() + data->size())
    {
        fail("invalid_pkcs12", "输入不是完整的 PKCS#12 数据包");
    }
    const password_text converted(password);
    const char* text = converted.data();
    if (PKCS12_mac_present(package.get()) && PKCS12_verify_mac(package.get(), text, -1) != 1)
    {
        // PKCS#12 区分空字符串和 NULL 密码，兼容两种空密码编码。
        if (*text != '\0' || PKCS12_verify_mac(package.get(), nullptr, 0) != 1)
        {
            fail("invalid_password_or_data", "PKCS#12 密码或数据无效");
        }
        text = nullptr;
    }
    std::unique_ptr<STACK_OF(PKCS7), safes_closer> safes(PKCS12_unpack_authsafes(package.get()));
    if (!safes)
    {
        fail("invalid_pkcs12", "PKCS#12 内容无效");
    }
    imported_identity identity;
    for (int index = 0; index < sk_PKCS7_num(safes.get()); ++index)
    {
        auto* safe = sk_PKCS7_value(safes.get(), index);
        std::unique_ptr<STACK_OF(PKCS12_SAFEBAG), bags_closer> bags;
        if (PKCS7_type_is_data(safe))
        {
            bags.reset(PKCS12_unpack_p7data(safe));
        }
        else if (PKCS7_type_is_encrypted(safe))
        {
            bags.reset(PKCS12_unpack_p7encdata(safe, text, text ? -1 : 0));
        }
        if (!bags)
        {
            fail("invalid_password_or_data", "PKCS#12 密码或数据无效");
        }
        read_bags(bags.get(), text, identity, 0);
    }
    if (identity.certificates.empty())
    {
        fail("invalid_pkcs12", "PKCS#12 数据包没有证书");
    }
    if (identity.key)
    {
        const auto owner = std::find_if(identity.certificates.begin(), identity.certificates.end(),
            [&](const cert_ptr& item)
            {
                return X509_check_private_key(item.get(), identity.key.get()) == 1;
            });
        if (owner == identity.certificates.end())
        {
            fail("invalid_pkcs12", "PKCS#12 私钥没有匹配的证书");
        }
        std::iter_swap(identity.certificates.begin(), owner);
    }
    return identity;
}

} // namespace

std::vector<byte_value> parse_pkcs12(const byte_value& data, const secret::handle& password)
{
    auto identity = import_package(data, password);
    std::vector<byte_value> result;
    for (const auto& item : identity.certificates)
    {
        result.push_back(certificate_bytes(item.get()));
    }
    return result;
}

secret::handle pkcs12_private_key(const byte_value& data, const secret::handle& password)
{
    auto identity = import_package(data, password);
    if (!identity.key)
    {
        fail("no_private_key", "PKCS#12 数据包没有私钥");
    }
    std::unique_ptr<PKCS8_PRIV_KEY_INFO, private_info_closer> info(EVP_PKEY2PKCS8(identity.key.get()));
    const int length = info ? i2d_PKCS8_PRIV_KEY_INFO(info.get(), nullptr) : -1;
    if (length <= 0 || static_cast<std::size_t>(length) > max_certificate_bytes)
    {
        fail("unsupported_key", "PKCS#12 私钥算法或导出方式不受支持");
    }
    auto result = std::make_shared<secret::buffer>(static_cast<std::size_t>(length));
    auto* cursor = result->writable().data();
    if (i2d_PKCS8_PRIV_KEY_INFO(info.get(), &cursor) != length)
    {
        fail("unsupported_key", "PKCS#12 私钥导出失败");
    }
    return result;
}

} // namespace tx_generated::x509
