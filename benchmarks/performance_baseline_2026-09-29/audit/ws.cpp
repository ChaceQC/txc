#include <windows.h>
#include <winhttp.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

struct http_handle
{
    HINTERNET value;

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

std::int64_t websocket_echo()
{
    std::int64_t checksum = 0;
    for (int i = 0; i < 100; ++i)
    {
        http_handle session(WinHttpOpen(L"TX/1.0",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, nullptr, nullptr, 0));
        WinHttpSetTimeouts(session.value, 5000, 5000, 5000, 5000);
        http_handle connection(WinHttpConnect(session.value,
            L"127.0.0.1", 19791, 0));
        http_handle request(WinHttpOpenRequest(connection.value, L"GET",
            L"/", nullptr, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES, 0));
        if (!WinHttpSetOption(request.value,
            WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0) ||
            !WinHttpSendRequest(request.value, WINHTTP_NO_ADDITIONAL_HEADERS,
                0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
            !WinHttpReceiveResponse(request.value, nullptr))
        {
            throw std::runtime_error("WebSocket handshake failed");
        }
        http_handle socket(WinHttpWebSocketCompleteUpgrade(request.value, 0));
        char buffer[8]{};
        DWORD received = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE type{};
        if (WinHttpWebSocketSend(socket.value,
            WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
            const_cast<char*>("ok"), 2) != NO_ERROR ||
            WinHttpWebSocketReceive(socket.value, buffer, sizeof(buffer),
                &received, &type) != NO_ERROR)
        {
            throw std::runtime_error("WebSocket exchange failed");
        }
        checksum += type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE &&
            received == 2 && std::string(buffer, 2) == "ok" ? 1 : 0;
        WinHttpWebSocketClose(socket.value,
            WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0);
    }
    return checksum;
}

int main()
{
    const auto start = std::chrono::steady_clock::now();
    const auto checksum = websocket_echo();
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start).count();
    std::cout << "websocket_echo\n" << elapsed << '\n' << checksum << '\n';
}
