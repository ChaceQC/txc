#include "stdlib/ws.hpp"

#include "stdlib/encoding.hpp"

#include <algorithm>
#include <chrono>
#include <future>
#include <limits>
#include <thread>

namespace tx_generated
{

std::shared_ptr<ws_connection_state> ws_client_connect(std::string_view url,
                                                        std::int64_t timeout_ms)
{
    if (timeout_ms <= 0 || timeout_ms > std::numeric_limits<int>::max())
    {
        network::fail("invalid_argument", "WebSocket 客户端超时必须为正数");
    }
    const auto address = network::parse_url(url, true);
    auto state = std::make_shared<ws_connection_state>();
    state->session.reset(WinHttpOpen(L"TX/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!state->session.get())
    {
        network::http_failure("创建 WebSocket 会话");
    }
    const auto timeout = static_cast<int>(timeout_ms);
    if (!WinHttpSetTimeouts(state->session.get(), timeout, timeout, timeout, timeout))
    {
        network::http_failure("设置 WebSocket 超时");
    }
    state->connection.reset(WinHttpConnect(state->session.get(),
        address.host.c_str(), address.port, 0));
    if (!state->connection.get())
    {
        network::http_failure("连接 WebSocket 主机");
    }
    network::http_handle request(WinHttpOpenRequest(state->connection.get(), L"GET",
        address.target.c_str(), nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, address.secure ? WINHTTP_FLAG_SECURE : 0));
    if (!request.get() || !WinHttpSetOption(request.get(),
        WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0))
    {
        network::http_failure("创建 WebSocket 握手请求");
    }
    if (!WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.get(), nullptr))
    {
        network::http_failure("WebSocket 握手");
    }
    DWORD status = 0;
    DWORD size = sizeof(status);
    if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE |
                             WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &size,
                             WINHTTP_NO_HEADER_INDEX))
    {
        network::http_failure("读取 WebSocket 握手状态");
    }
    if (status != 101)
    {
        network::fail("protocol_error", "WebSocket 服务端未接受升级请求");
    }
    // 握手已结束；单次 receive 的正数超时由工作线程控制，0 需要无限等待。
    if (!WinHttpSetTimeouts(request.get(), timeout, timeout, timeout, 0))
    {
        network::http_failure("设置 WebSocket 读取等待方式");
    }
    state->websocket.reset(WinHttpWebSocketCompleteUpgrade(request.get(), 0));
    if (!state->websocket.get())
    {
        network::http_failure("完成 WebSocket 升级");
    }
    return state;
}

void ws_client_send(ws_connection_state& state, std::string_view data,
                    bool binary)
{
    const auto result = WinHttpWebSocketSend(state.websocket.get(),
        binary ? WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE
               : WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
        data.empty() ? nullptr : const_cast<char*>(data.data()),
        static_cast<DWORD>(data.size()));
    if (result != NO_ERROR)
    {
        network::fail(result == ERROR_WINHTTP_TIMEOUT ? "timeout" : "operation_failed",
                      "发送 WebSocket 文本失败，WinHTTP 错误码 " +
                      std::to_string(result));
    }
}

namespace
{

ws_message_data read_client_message(HINTERNET websocket, bool binary)
{
    std::string text;
    while (true)
    {
        char buffer[8192];
        DWORD received = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE type{};
        const auto result = WinHttpWebSocketReceive(websocket, buffer,
            sizeof(buffer), &received, &type);
        if (result != NO_ERROR)
        {
            network::fail(result == ERROR_WINHTTP_TIMEOUT ? "timeout" : "operation_failed",
                          "读取 WebSocket 消息失败，WinHTTP 错误码 " +
                          std::to_string(result));
        }
        if (type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE)
        {
            USHORT status = 1005;
            char reason[123]{};
            DWORD used = 0;
            const auto queried = WinHttpWebSocketQueryCloseStatus(websocket,
                &status, reason, sizeof(reason), &used);
            if (queried != NO_ERROR)
            {
                network::fail("protocol_error", "无法读取 WebSocket 关闭状态");
            }
            std::string text(reason, used);
            network::validate_utf8(text);
            return {false, {}, status, std::move(text)};
        }
        const auto complete = binary
            ? WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE
            : WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE;
        const auto fragment = binary
            ? WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE
            : WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE;
        if (type != complete && type != fragment)
        {
            network::fail("unsupported_frame", "收到与读取接口类型不符的 WebSocket 消息");
        }
        if (text.size() + received > network::max_body_bytes)
        {
            network::fail("size_limit", "WebSocket 文本消息超过 8 MiB");
        }
        text.append(buffer, received);
        if (type == complete)
        {
            if (!binary)
            {
                network::validate_utf8(text);
            }
            return {true, std::move(text), 1005, {}};
        }
    }
}

} // namespace

ws_message_data ws_client_receive(std::shared_ptr<ws_connection_state> state,
                                  std::int64_t timeout_ms, bool binary)
{
    const auto websocket = state->websocket.get();
    if (timeout_ms == 0)
    {
        auto value = read_client_message(websocket, binary);
        if (!value.open)
        {
            state->close_status = {true, value.close_code, value.close_reason};
        }
        return value;
    }
    if (timeout_ms > std::numeric_limits<int>::max())
    {
        network::fail("invalid_argument", "WebSocket 读取超时过大");
    }
    // WinHTTP 不接受在升级后的句柄上修改读取超时；单次读取在工作线程中等待。
    auto completion = std::make_shared<std::promise<ws_message_data>>();
    auto ready = completion->get_future();
    std::thread([completion, state, websocket, binary]()
    {
        try
        {
            completion->set_value(read_client_message(websocket, binary));
        }
        catch (...)
        {
            completion->set_exception(std::current_exception());
        }
    }).detach();
    if (ready.wait_for(std::chrono::milliseconds(timeout_ms)) !=
        std::future_status::ready)
    {
        state->websocket.reset();
        state->open = false;
        network::fail("timeout", "读取 WebSocket 消息超时");
    }
    auto value = ready.get();
    if (!value.open)
    {
        state->close_status = {true, value.close_code, value.close_reason};
    }
    return value;
}

void ws_client_close(ws_connection_state& state, std::uint16_t code,
                     std::string_view reason) noexcept
{
    if (state.open && state.websocket.get())
    {
        WinHttpWebSocketClose(state.websocket.get(), code,
            reason.empty() ? nullptr : const_cast<char*>(reason.data()),
            static_cast<DWORD>(reason.size()));
    }
    state.websocket.reset();
    state.open = false;
}

} // namespace tx_generated
