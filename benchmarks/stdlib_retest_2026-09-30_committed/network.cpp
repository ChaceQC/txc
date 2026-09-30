#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "common.hpp"

namespace
{

template<class function>
function symbol(HMODULE module, const char* name)
{
    auto result = reinterpret_cast<function>(GetProcAddress(module, name));
    if (!result)
    {
        throw std::runtime_error("OpenSSL symbol unavailable");
    }
    return result;
}

void tls_handshakes()
{
    auto library = LoadLibraryA("libssl-3-x64.dll");
    auto method = symbol<const void*(*)()>(library, "TLS_client_method");
    auto new_context = symbol<void*(*)(const void*)>(library, "SSL_CTX_new");
    auto free_context = symbol<void(*)(void*)>(library, "SSL_CTX_free");
    auto trust = symbol<int(*)(void*, const char*, const char*)>(library, "SSL_CTX_load_verify_locations");
    auto verify = symbol<void(*)(void*, int, void*)>(library, "SSL_CTX_set_verify");
    auto create = symbol<void*(*)(void*)>(library, "SSL_new");
    auto destroy = symbol<void(*)(void*)>(library, "SSL_free");
    auto hostname = symbol<int(*)(void*, const char*)>(library, "SSL_set1_host");
    auto descriptor = symbol<int(*)(void*, int)>(library, "SSL_set_fd");
    auto handshake = symbol<int(*)(void*)>(library, "SSL_connect");
    auto write = symbol<int(*)(void*, const void*, int)>(library, "SSL_write");
    auto read = symbol<int(*)(void*, void*, int)>(library, "SSL_read");
    auto shutdown = symbol<int(*)(void*)>(library, "SSL_shutdown");
    auto alpn = symbol<int(*)(void*, const unsigned char*, unsigned int)>(library, "SSL_set_alpn_protos");
    auto context = new_context(method());
    if (!context || trust(context, "ca.pem", nullptr) != 1)
    {
        throw std::runtime_error("TLS trust unavailable");
    }
    verify(context, 1, nullptr);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<u_short>(std::stoi(std::getenv("BENCH_TLS_PORT"))));
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    auto start = bench_clock::now();
    std::int64_t total = 0;
    for (int i = 0; i < 10; ++i)
    {
        SOCKET peer = socket(AF_INET, SOCK_STREAM, 0);
        DWORD timeout = 5000;
        setsockopt(peer, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        setsockopt(peer, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        if (connect(peer, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
        {
            throw std::runtime_error("TLS TCP connect failed");
        }
        auto ssl = create(context);
        const unsigned char protocols[] = {5, 'b', 'e', 'n', 'c', 'h'};
        alpn(ssl, protocols, sizeof(protocols));
        hostname(ssl, "localhost");
        descriptor(ssl, static_cast<int>(peer));
        char reply = 0;
        if (handshake(ssl) != 1 || write(ssl, "*", 1) != 1 || read(ssl, &reply, 1) != 1 || reply != '*')
        {
            throw std::runtime_error("TLS verified echo failed");
        }
        ++total;
        shutdown(ssl);
        destroy(ssl);
        closesocket(peer);
    }
    report("tls_handshake", start, total);
    free_context(context);
    FreeLibrary(library);
}

} // namespace

void bench_network()
{
    WSADATA data{};
    WSAStartup(MAKEWORD(2, 2), &data);
    auto start = bench_clock::now();
    std::int64_t total = 0;
    for (int i = 0; i < 100; ++i)
    {
        addrinfo* result = nullptr;
        addrinfo hints{};
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo("localhost", nullptr, &hints, &result) != 0)
        {
            throw std::runtime_error("DNS failed");
        }
        total += result != nullptr;
        freeaddrinfo(result);
    }
    report("dns_localhost", start, total);
    auto payload = read_file("message.bin");
    SOCKET udp = socket(AF_INET, SOCK_DGRAM, 0);
    DWORD timeout = 5000;
    setsockopt(udp, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<u_short>(std::stoi(std::getenv("BENCH_UDP_PORT"))));
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    start = bench_clock::now();
    total = 0;
    for (int i = 0; i < 500; ++i)
    {
        sendto(udp, reinterpret_cast<const char*>(payload.data()), payload.size(), 0,
            reinterpret_cast<sockaddr*>(&address), sizeof(address));
        std::vector<std::uint8_t> reply(2048);
        auto length = recvfrom(udp, reinterpret_cast<char*>(reply.data()), reply.size(), 0, nullptr, nullptr);
        if (length < 0)
        {
            throw std::runtime_error("UDP failed");
        }
        reply.resize(length);
        if (reply == payload)
        {
            total += reply.size();
        }
    }
    report("udp_echo", start, total);
    closesocket(udp);
    auto pipe_name = std::string("\\\\.\\pipe\\tx-ipc-") + std::getenv("BENCH_PIPE");
    WaitNamedPipeA(pipe_name.c_str(), 5000);
    HANDLE pipe = CreateFileA(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
        OPEN_EXISTING, 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE)
    {
        throw std::runtime_error("IPC connection failed");
    }
    start = bench_clock::now();
    total = 0;
    for (int i = 0; i < 500; ++i)
    {
        // 固定 CBOR 整数与相同 TXIP 帧；不把此专项解释成任意 CBOR 对象的编解码基准。
        std::vector<std::uint8_t> frame{'T', 'X', 'I', 'P', 1, 0, 0, 0,
            1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0x18, 42};
        DWORD written = 0;
        if (!WriteFile(pipe, frame.data(), frame.size(), &written, nullptr) || written != frame.size())
        {
            throw std::runtime_error("IPC write failed");
        }
        std::vector<std::uint8_t> reply(22);
        std::size_t received = 0;
        while (received < reply.size())
        {
            DWORD count = 0;
            if (!ReadFile(pipe, reply.data() + received, reply.size() - received, &count, nullptr) || !count)
            {
                throw std::runtime_error("IPC read failed");
            }
            received += count;
        }
        if (reply != frame)
        {
            throw std::runtime_error("IPC mismatch");
        }
        total += reply.back();
    }
    report("ipc_echo", start, total);
    CloseHandle(pipe);
    tls_handshakes();
    WSACleanup();
}
