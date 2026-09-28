#include "stdlib/dns.hpp"

#include "stdlib/error.hpp"
#include "stdlib/network_common.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <string>
#include <unordered_set>

#ifdef _WIN32
#include <windns.h>
#include <ws2tcpip.h>

// 当前随项目使用的 MinGW 头文件只声明了 DNS_QUERY_RESULT，导入库已有
// DnsQueryEx/DnsCancelQuery；这里补齐 Windows SDK 的 v1 请求布局。
struct dns_query_request_compat
{
    ULONG Version;
    PCWSTR QueryName;
    WORD QueryType;
    ULONG64 QueryOptions;
    void* pDnsServerList;
    ULONG InterfaceIndex;
    PDNS_QUERY_COMPLETION_ROUTINE pQueryCompletionCallback;
    void* pQueryContext;
};

struct dns_query_cancel_compat
{
    CHAR Reserved[32];
};

static_assert(sizeof(dns_query_request_compat) == 64);
static_assert(sizeof(dns_query_cancel_compat) == 32);

extern "C" DNS_STATUS WINAPI DnsQueryEx(dns_query_request_compat*,
    DNS_QUERY_RESULT*, dns_query_cancel_compat*);
extern "C" DNS_STATUS WINAPI DnsCancelQuery(dns_query_cancel_compat*);
#endif

namespace tx_generated::dns
{
namespace
{

using clock_type = std::chrono::steady_clock;

enum class stop_reason
{
    none,
    cancelled,
    deadline
};

stop_reason token_status(const std::shared_ptr<cancellation_state>& token)
{
    if (!token)
    {
        return stop_reason::none;
    }
    std::lock_guard lock(token->mutex);
    if (token->cancelled)
    {
        return stop_reason::cancelled;
    }
    return token->deadline && clock_type::now() >= *token->deadline
        ? stop_reason::deadline : stop_reason::none;
}

void check_token(const std::shared_ptr<cancellation_state>& token)
{
    if (const auto status = token_status(token); status != stop_reason::none)
    {
        throw runtime_failure({tx::error_kind::cancelled,
            status == stop_reason::cancelled ? "cancelled" : "deadline_exceeded",
            status == stop_reason::cancelled ? "DNS 解析已取消" : "DNS 解析截止时间已到"});
    }
}

#ifdef _WIN32

class event_handle
{
public:
    event_handle() : value_(CreateEventW(nullptr, TRUE, FALSE, nullptr))
    {
        if (!value_)
        {
            network::fail("operation_failed", "无法创建 DNS 查询完成事件");
        }
    }

    ~event_handle()
    {
        CloseHandle(value_);
    }

    [[nodiscard]] HANDLE get() const noexcept
    {
        return value_;
    }

private:
    HANDLE value_;
};

class query_result
{
public:
    query_result()
    {
        value.Version = 1;
    }

    ~query_result()
    {
        if (value.pQueryRecords)
        {
            DnsRecordListFree(value.pQueryRecords, DnsFreeRecordList);
        }
    }

    DNS_QUERY_RESULT value{};
};

struct query_context
{
    HANDLE completed;
};

void WINAPI finish_query(void* context, DNS_QUERY_RESULT*)
{
    SetEvent(static_cast<query_context*>(context)->completed);
}

std::wstring wide_host(std::string_view host)
{
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        host.data(), static_cast<int>(host.size()), nullptr, 0);
    if (length <= 0)
    {
        network::fail("invalid_utf8", "DNS 主机名不是有效 UTF-8");
    }
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, host.data(),
            static_cast<int>(host.size()), result.data(), length) != length)
    {
        network::fail("invalid_utf8", "DNS 主机名无法转换为系统编码");
    }
    return result;
}

std::string ip_text(int family, const void* value)
{
    char buffer[INET6_ADDRSTRLEN]{};
    if (!InetNtopA(family, const_cast<void*>(value), buffer, sizeof(buffer)))
    {
        network::fail("operation_failed", "无法格式化 DNS 地址");
    }
    return buffer;
}

void append_record(std::vector<address>& result, std::unordered_set<std::string>& seen,
                   const DNS_RECORD* record, WORD type)
{
    if (record->wType != type)
    {
        return;
    }
    const int family = type == DNS_TYPE_A ? AF_INET : AF_INET6;
    const void* bytes = type == DNS_TYPE_A
        ? static_cast<const void*>(&record->Data.A.IpAddress)
        : static_cast<const void*>(&record->Data.AAAA.Ip6Address);
    auto ip = ip_text(family, bytes);
    if (seen.insert(ip).second)
    {
        if (result.size() == 128)
        {
            network::fail("size_limit", "DNS 地址记录超过 128 条");
        }
        result.push_back({std::move(ip), family == AF_INET ? "ipv4" : "ipv6",
            record->dwTtl});
    }
}

DNS_STATUS query_type(const std::wstring& host, WORD type,
                      clock_type::time_point deadline,
                      const std::shared_ptr<cancellation_state>& token,
                      std::vector<address>& addresses,
                      std::unordered_set<std::string>& seen)
{
    check_token(token);
    if (clock_type::now() >= deadline)
    {
        network::fail("timeout", "DNS 解析超时");
    }
    event_handle completed;
    query_context context{completed.get()};
    dns_query_request_compat request{};
    request.Version = 1;
    request.QueryName = host.c_str();
    request.QueryType = type;
    request.pQueryCompletionCallback = finish_query;
    request.pQueryContext = &context;
    query_result result;
    dns_query_cancel_compat cancellation{};
    const auto started = DnsQueryEx(&request, &result.value, &cancellation);
    if (started == DNS_REQUEST_PENDING)
    {
        while (WaitForSingleObject(completed.get(), 0) != WAIT_OBJECT_0)
        {
            const auto stopped = token_status(token);
            const auto expired = clock_type::now() >= deadline;
            if (stopped != stop_reason::none || expired)
            {
                DnsCancelQuery(&cancellation);
                // 回调完成后才能释放查询上下文、记录和事件。
                WaitForSingleObject(completed.get(), INFINITE);
                if (stopped != stop_reason::none)
                {
                    check_token(token);
                }
                network::fail("timeout", "DNS 解析超时");
            }
            WaitForSingleObject(completed.get(), 10);
        }
    }
    else if (started != ERROR_SUCCESS)
    {
        return started;
    }
    const auto status = result.value.QueryStatus;
    if (status == ERROR_SUCCESS)
    {
        for (auto* record = result.value.pQueryRecords; record;
             record = record->pNext)
        {
            append_record(addresses, seen, record, type);
        }
    }
    return status;
}

void check_host(std::string_view host)
{
    network::validate_utf8(host);
    if (host.empty() || host.size() > 253 ||
        host.find_first_of("/\\@?#\0 \t\r\n", 0, 10) != std::string_view::npos)
    {
        network::fail("invalid_argument", "DNS 主机名为空、过长或包含非法字符");
    }
}

std::vector<address> literal_address(const std::wstring& host)
{
    IN_ADDR ipv4{};
    if (InetPtonW(AF_INET, host.c_str(), &ipv4) == 1)
    {
        return {{ip_text(AF_INET, &ipv4), "ipv4", 0}};
    }
    IN6_ADDR ipv6{};
    if (InetPtonW(AF_INET6, host.c_str(), &ipv6) == 1)
    {
        return {{ip_text(AF_INET6, &ipv6), "ipv6", 0}};
    }
    if (host.find(L':') != std::wstring::npos)
    {
        network::fail("invalid_argument", "DNS 主机名不能包含端口或无效的 IPv6 地址");
    }
    if (host == L"localhost")
    {
        return {{"127.0.0.1", "ipv4", 0}, {"::1", "ipv6", 0}};
    }
    return {};
}

#endif

} // namespace

std::vector<address> resolve(std::string_view host, std::int64_t timeout_ms,
                             const std::shared_ptr<cancellation_state>& token)
{
#ifdef _WIN32
    check_host(host);
    if (timeout_ms < 1 || timeout_ms > 60000)
    {
        network::fail("invalid_argument", "DNS 解析超时必须为 1～60000 毫秒");
    }
    check_token(token);
    const auto wide = wide_host(host);
    if (auto literal = literal_address(wide); !literal.empty())
    {
        return literal;
    }
    const auto deadline = clock_type::now() +
        std::chrono::milliseconds(timeout_ms);
    std::vector<address> addresses;
    std::unordered_set<std::string> seen;
    const auto status_a = query_type(wide, DNS_TYPE_A, deadline,
        token, addresses, seen);
    const auto status_aaaa = query_type(wide, DNS_TYPE_AAAA, deadline,
        token, addresses, seen);
    if (!addresses.empty())
    {
        return addresses;
    }
    if (status_a == DNS_ERROR_RCODE_NAME_ERROR &&
        status_aaaa == DNS_ERROR_RCODE_NAME_ERROR)
    {
        network::fail("name_not_found", "DNS 主机名不存在");
    }
    if ((status_a == DNS_INFO_NO_RECORDS || status_a == ERROR_SUCCESS ||
         status_a == DNS_ERROR_RCODE_NAME_ERROR) &&
        (status_aaaa == DNS_INFO_NO_RECORDS || status_aaaa == ERROR_SUCCESS ||
         status_aaaa == DNS_ERROR_RCODE_NAME_ERROR))
    {
        network::fail("no_records", "DNS 主机名没有可用的 IPv4 或 IPv6 地址");
    }
    network::fail("operation_failed", "DNS 地址解析失败");
#else
    (void)host;
    (void)timeout_ms;
    (void)token;
    network::fail("unsupported_platform", "此平台尚不支持 DNS 地址解析");
#endif
}

} // namespace tx_generated::dns
