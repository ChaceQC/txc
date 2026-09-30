#include "stdlib/http3_server_internal.hpp"

#include "stdlib/error.hpp"

#include <algorithm>
#include <array>
#include <limits>
#ifdef _WIN32
#include <ncrypt.h>
#include <ws2tcpip.h>
#else
#include "stdlib/x509.hpp"
#include <arpa/inet.h>
#endif
#include <utility>

namespace tx_generated::http3
{
namespace
{

#ifdef _WIN32
class password_text
{
public:
    explicit password_text(const secret::handle& password)
    {
        if (!password)
        {
            network::fail("invalid_argument", "HTTP/3 身份密码未提供");
        }
        const auto bytes = password->view();
        if (bytes.size() > 4096 ||
            std::find(bytes.begin(), bytes.end(), 0) != bytes.end())
        {
            network::fail("invalid_argument", "HTTP/3 身份密码长度无效");
        }
        if (bytes.empty())
        {
            value_.push_back(L'\0');
            return;
        }
        const auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<int>(bytes.size()), nullptr, 0);
        if (size < 1)
        {
            network::fail("invalid_argument", "HTTP/3 身份密码不是 UTF-8");
        }
        value_.resize(static_cast<std::size_t>(size) + 1);
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                reinterpret_cast<const char*>(bytes.data()),
                static_cast<int>(bytes.size()), value_.data(), size) != size)
        {
            network::fail("operation_failed", "转换 HTTP/3 身份密码失败");
        }
        value_.back() = L'\0';
    }

    ~password_text()
    {
        SecureZeroMemory(value_.data(), value_.size() * sizeof(wchar_t));
    }

    [[nodiscard]] const wchar_t* data() const noexcept
    {
        return value_.data();
    }

private:
    std::vector<wchar_t> value_;
};
#else
class password_text
{
public:
    explicit password_text(const secret::handle& password)
    {
        if (!password)
        {
            network::fail("invalid_argument", "HTTP/3 身份密码未提供");
        }
        const auto bytes = password->view();
        if (bytes.size() > 4096 || std::find(bytes.begin(), bytes.end(), 0) != bytes.end())
        {
            network::fail("invalid_argument", "HTTP/3 身份密码长度无效");
        }
        network::validate_utf8({reinterpret_cast<const char*>(bytes.data()), bytes.size()});
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
#endif

QUIC_ADDR bind_address(std::string_view host, std::int64_t port)
{
    network::initialize_winsock();
    network::validate_utf8(host);
    if (port < 1 || port > 65535 || host.find('\0') != std::string_view::npos)
    {
        network::fail("invalid_argument", "HTTP/3 监听地址或端口无效");
    }
    QUIC_ADDR result{};
    const std::string text(host);
    if (host.empty() || inet_pton(AF_INET, text.c_str(),
                                   &result.Ipv4.sin_addr) == 1)
    {
        result.Ipv4.sin_family = AF_INET;
        result.Ipv4.sin_port = htons(static_cast<u_short>(port));
    }
    else if (inet_pton(AF_INET6, text.c_str(),
                        &result.Ipv6.sin6_addr) == 1)
    {
        result.Ipv6.sin6_family = AF_INET6;
        result.Ipv6.sin6_port = htons(static_cast<u_short>(port));
    }
    else
    {
        network::fail("invalid_argument", "HTTP/3 监听地址须为本机数值 IP");
    }
    return result;
}

#ifdef _WIN32
void delete_imported_keys(HCERTSTORE store) noexcept
{
    if (!store)
    {
        return;
    }
    try
    {
        PCCERT_CONTEXT cursor = nullptr;
        while ((cursor = CertEnumCertificatesInStore(store, cursor)) != nullptr)
        {
            DWORD size = 0;
            if (!CertGetCertificateContextProperty(cursor,
                    CERT_KEY_PROV_INFO_PROP_ID, nullptr, &size) || size == 0)
            {
                continue;
            }
            std::vector<std::uint8_t> storage(size);
            if (!CertGetCertificateContextProperty(cursor,
                    CERT_KEY_PROV_INFO_PROP_ID, storage.data(), &size))
            {
                continue;
            }
            const auto* info = reinterpret_cast<const CRYPT_KEY_PROV_INFO*>(
                storage.data());
            if (!info->pwszProvName || !info->pwszContainerName)
            {
                continue;
            }
            NCRYPT_PROV_HANDLE provider = 0;
            if (NCryptOpenStorageProvider(&provider, info->pwszProvName, 0) != 0)
            {
                continue;
            }
            NCRYPT_KEY_HANDLE key = 0;
            const DWORD flags = (info->dwFlags & CRYPT_MACHINE_KEYSET)
                ? NCRYPT_MACHINE_KEY_FLAG : 0;
            if (NCryptOpenKey(provider, &key, info->pwszContainerName,
                              0, flags) == 0 &&
                NCryptDeleteKey(key, 0) != 0)
            {
                NCryptFreeObject(key);
            }
            NCryptFreeObject(provider);
        }
    }
    catch (...)
    {
        // 析构路径不能再抛异常；后续仍会关闭证书存储。
    }
}
#endif

} // namespace

server_listener::server_listener(std::string_view host, std::int64_t port,
    const byte_value& package, const secret::handle& password)
    : library_(quic()), api_(library_.get())
{
    if (!package || package->empty() || package->size() > 4 * 1024 * 1024)
    {
        network::fail("invalid_argument", "HTTP/3 PKCS#12 身份为空或超过 4 MiB");
    }
    const auto address = bind_address(host, port);
    try
    {
#ifdef _WIN32
        CRYPT_DATA_BLOB blob{static_cast<DWORD>(package->size()),
            const_cast<BYTE*>(package->data())};
        if (!PFXIsPFXBlob(&blob))
        {
            network::fail("invalid_argument", "HTTP/3 身份不是 PKCS#12 包");
        }
        const password_text converted(password);
        // Schannel 的 QUIC 凭据需要可重新打开的 CNG 私钥；监听结束后删掉导入的容器。
        constexpr DWORD flags = PKCS12_ALWAYS_CNG_KSP | CRYPT_USER_KEYSET;
        certificate_store_ = PFXImportCertStore(&blob, converted.data(), flags);
        if (!certificate_store_)
        {
            network::fail("invalid_argument", "HTTP/3 身份密码或证书无效");
        }
        PCCERT_CONTEXT cursor = nullptr;
        while ((cursor = CertEnumCertificatesInStore(certificate_store_,
                                                      cursor)) != nullptr)
        {
            DWORD size = 0;
            if (CertGetCertificateContextProperty(cursor,
                    CERT_KEY_PROV_INFO_PROP_ID, nullptr, &size))
            {
                if (certificate_)
                {
                    network::fail("invalid_argument", "HTTP/3 身份包含多把私钥");
                }
                certificate_ = CertDuplicateCertificateContext(cursor);
            }
        }
        if (!certificate_)
        {
            network::fail("invalid_argument", "HTTP/3 身份缺少关联私钥");
        }
#else
        // 先执行统一 PKCS#12 校验，拒绝多私钥及证书不匹配，再把包交给 MsQuic。
        (void)x509::parse_pkcs12(package, password);
        auto private_key = x509::pkcs12_private_key(package, password);
        secret::close(private_key);
        const password_text converted(password);
#endif
        QUIC_SETTINGS settings{};
        settings.IsSet.PeerBidiStreamCount = TRUE;
        settings.PeerBidiStreamCount = 16;
        settings.IsSet.PeerUnidiStreamCount = TRUE;
        settings.PeerUnidiStreamCount = 3;
        settings.IsSet.IdleTimeoutMs = TRUE;
        settings.IdleTimeoutMs = 30000;
        settings.IsSet.MigrationEnabled = TRUE;
        settings.MigrationEnabled = TRUE;
        settings.IsSet.ServerResumptionLevel = TRUE;
        settings.ServerResumptionLevel = QUIC_SERVER_NO_RESUME;
        static std::array<std::uint8_t, 2> protocol{'h', '3'};
        QUIC_BUFFER alpn{2, protocol.data()};
        require_quic(api_->ConfigurationOpen(library_.registration(), &alpn,
            1, &settings, sizeof(settings), nullptr, &configuration_),
            "配置 HTTP/3 服务端");
        QUIC_CREDENTIAL_CONFIG credential{};
#ifdef _WIN32
        credential.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_CONTEXT;
        credential.CertificateContext =
            const_cast<CERT_CONTEXT*>(certificate_);
#else
        // 同步加载把凭据复制进 TLS 上下文，不创建私钥文件或持久化容器。
        QUIC_CERTIFICATE_PKCS12 identity{package->data(),
            static_cast<std::uint32_t>(package->size()), converted.data()};
        credential.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_PKCS12;
        credential.CertificatePkcs12 = &identity;
#endif
        require_quic(api_->ConfigurationLoadCredential(configuration_,
            &credential), "加载 HTTP/3 服务端证书");
        require_quic(api_->ListenerOpen(library_.registration(), on_listener,
            this, &listener_), "创建 HTTP/3 监听器");
        require_quic(api_->ListenerStart(listener_, &alpn, 1, &address),
                     "启动 HTTP/3 监听器");
        reaper_ = std::jthread([this](std::stop_token token)
        {
            reap(token);
        });
    }
    catch (...)
    {
        if (listener_)
        {
            api_->ListenerClose(listener_);
        }
        if (configuration_)
        {
            api_->ConfigurationClose(configuration_);
        }
#ifdef _WIN32
        if (certificate_)
        {
            CertFreeCertificateContext(certificate_);
        }
        if (certificate_store_)
        {
            delete_imported_keys(certificate_store_);
            CertCloseStore(certificate_store_, 0);
        }
#endif
        throw;
    }
}

server_listener::~server_listener() noexcept
{
    close();
    if (configuration_)
    {
        api_->ConfigurationClose(configuration_);
    }
#ifdef _WIN32
    if (certificate_)
    {
        CertFreeCertificateContext(certificate_);
    }
    if (certificate_store_)
    {
        delete_imported_keys(certificate_store_);
        CertCloseStore(certificate_store_, 0);
    }
#endif
}

void server_listener::close() noexcept
{
    std::vector<std::shared_ptr<server_connection>> connections;
    std::deque<server_request_ticket> completed;
    {
        std::lock_guard lock(mutex_);
        if (closed_)
        {
            return;
        }
        closed_ = true;
        connections.swap(connections_);
        completed.swap(completed_);
        changed_.notify_all();
    }
    completed.clear();
    if (reaper_.joinable())
    {
        reaper_.request_stop();
        reaper_changed_.notify_all();
        reaper_.join();
    }
    if (listener_)
    {
        api_->ListenerStop(listener_);
        api_->ListenerClose(listener_);
        listener_ = nullptr;
    }
    for (const auto& connection : connections)
    {
        connection->close_unclaimed_requests();
    }
    connections.clear();
}

void server_listener::reap(std::stop_token token)
{
    while (!token.stop_requested())
    {
        {
            std::unique_lock lock(reaper_mutex_);
            reaper_changed_.wait_for(lock, std::chrono::milliseconds(100), [&]
            {
                return token.stop_requested();
            });
        }
        if (token.stop_requested())
        {
            break;
        }
        std::size_t connection_count = 0;
        {
            std::lock_guard lock(mutex_);
            connection_count = connections_.size();
        }
        for (std::size_t index = 0; index < connection_count; ++index)
        {
            std::shared_ptr<server_connection> connection;
            {
                std::lock_guard lock(mutex_);
                if (closed_ || index >= connections_.size())
                {
                    break;
                }
                connection = connections_[index];
            }
            connection->reap_native_handles();
        }
        std::vector<std::shared_ptr<server_connection>> retired;
        {
            std::lock_guard lock(mutex_);
            for (auto item = connections_.begin(); item != connections_.end();)
            {
                if ((*item)->stopped())
                {
                    retired.push_back(std::move(*item));
                    item = connections_.erase(item);
                }
                else
                {
                    ++item;
                }
            }
        }
        retired.clear();
    }
}

void server_listener::queue(server_request_ticket request)
{
    std::lock_guard lock(mutex_);
    if (closed_ || completed_.size() >= 64)
    {
        network::fail("size_limit", "HTTP/3 待处理请求队列已满或监听器关闭");
    }
    completed_.push_back(std::move(request));
    changed_.notify_one();
}

void server_listener::discard(const server_connection* connection,
                              std::int64_t stream_id)
{
    std::vector<server_request_ticket> discarded;
    {
        std::lock_guard lock(mutex_);
        for (auto item = completed_.begin(); item != completed_.end();)
        {
            if (item->connection.get() == connection &&
                (stream_id < 0 || item->request->stream_id == stream_id))
            {
                discarded.push_back(std::move(*item));
                item = completed_.erase(item);
            }
            else
            {
                ++item;
            }
        }
        changed_.notify_all();
    }
}

void server_listener::request_reap() noexcept
{
    reaper_changed_.notify_one();
}

void server_listener::report_error(std::exception_ptr error) noexcept
{
    std::lock_guard lock(mutex_);
    if (!error_)
    {
        error_ = error;
    }
    changed_.notify_all();
}

server_request_ticket server_listener::pop(std::int64_t timeout_ms)
{
    if (timeout_ms < 0 || timeout_ms > 60000)
    {
        network::fail("invalid_argument", "HTTP/3 等待超时参数无效");
    }
    std::unique_lock lock(mutex_);
    const auto ready = [&]
    {
        return closed_ || error_ || !completed_.empty();
    };
    const bool arrived = timeout_ms == 0
        ? (changed_.wait(lock, ready), true)
        : changed_.wait_for(lock, std::chrono::milliseconds(timeout_ms), ready);
    if (!arrived)
    {
        network::fail("timeout", "等待 HTTP/3 请求超时");
    }
    if (error_)
    {
        auto error = std::exchange(error_, {});
        std::rethrow_exception(error);
    }
    if (completed_.empty())
    {
        network::fail("connection_closed", "HTTP/3 监听器已关闭");
    }
    auto result = std::move(completed_.front());
    completed_.pop_front();
    return result;
}

QUIC_STATUS QUIC_API server_listener::on_listener(HQUIC, void* context,
                                                    QUIC_LISTENER_EVENT* event)
{
    try
    {
        return static_cast<server_listener*>(context)->event(event);
    }
    catch (...)
    {
        return QUIC_STATUS_INTERNAL_ERROR;
    }
}

QUIC_STATUS server_listener::event(QUIC_LISTENER_EVENT* event)
{
    if (event->Type != QUIC_LISTENER_EVENT_NEW_CONNECTION)
    {
        return QUIC_STATUS_SUCCESS;
    }
    {
        std::lock_guard lock(mutex_);
        const auto active = std::count_if(connections_.begin(),
            connections_.end(), [](const auto& connection)
            {
                return !connection->stopped();
            });
        if (closed_ || active >= 16)
        {
            return QUIC_STATUS_CONNECTION_REFUSED;
        }
    }
    auto session = std::make_shared<server_connection>(shared_from_this(),
        event->NEW_CONNECTION.Connection);
    bool closed = false;
    {
        std::lock_guard lock(mutex_);
        if (closed_)
        {
            closed = true;
        }
        else
        {
            connections_.push_back(session);
        }
    }
    if (closed)
    {
        session->reject_failed_configuration();
        return QUIC_STATUS_CONNECTION_REFUSED;
    }
    const auto status = api_->ConnectionSetConfiguration(
        event->NEW_CONNECTION.Connection, configuration_);
    if (QUIC_FAILED(status))
    {
        session->reject_failed_configuration();
    }
    return status;
}

} // namespace tx_generated::http3
