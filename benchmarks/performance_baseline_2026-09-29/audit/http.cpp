#include <windows.h>
#include <winhttp.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

struct http_handle
{
    HINTERNET value = nullptr;

    explicit http_handle(HINTERNET handle) : value(handle)
    {
        if (!value)
        {
            throw std::runtime_error("WinHTTP handle failed");
        }
    }

    ~http_handle()
    {
        WinHttpCloseHandle(value);
    }
};

std::int64_t http_get()
{
    std::int64_t checksum = 0;
    for (int i = 0; i < 100; ++i)
    {
        http_handle session(WinHttpOpen(L"TX/1.0",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, nullptr, nullptr, 0));
        http_handle connection(WinHttpConnect(session.value, L"127.0.0.1",
            19790, 0));
        http_handle request(WinHttpOpenRequest(connection.value, L"GET",
            L"/data", nullptr, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES, 0));
        WinHttpSetTimeouts(request.value, 5000, 5000, 5000, 5000);
        if (!WinHttpSendRequest(request.value, WINHTTP_NO_ADDITIONAL_HEADERS,
            0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
            !WinHttpReceiveResponse(request.value, nullptr))
        {
            throw std::runtime_error("WinHTTP request failed");
        }
        DWORD status = 0;
        DWORD status_size = sizeof(status);
        if (!WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE |
            WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
            &status, &status_size, WINHTTP_NO_HEADER_INDEX))
        {
            throw std::runtime_error("WinHTTP status failed");
        }
        char body[2]{};
        DWORD received = 0;
        if (!WinHttpReadData(request.value, body, 2, &received))
        {
            throw std::runtime_error("WinHTTP body failed");
        }
        checksum += status == 200 && received == 2 &&
            std::string(body, 2) == "OK" ? 1 : 0;
    }
    return checksum;
}

int main()
{
    const auto start = std::chrono::steady_clock::now();
    const auto checksum = http_get();
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start).count();
    std::cout << "http_get\n" << elapsed << '\n' << checksum << '\n';
}
