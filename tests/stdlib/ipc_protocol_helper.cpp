#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace
{

bool read_exact(HANDLE source, std::uint8_t* data, std::size_t size)
{
    while (size > 0)
    {
        DWORD count = 0;
        if (!ReadFile(source, data, static_cast<DWORD>(size), &count,
                      nullptr) || count == 0)
        {
            return false;
        }
        data += count;
        size -= count;
    }
    return true;
}

bool write_exact(HANDLE target, const std::uint8_t* data, std::size_t size)
{
    while (size > 0)
    {
        DWORD count = 0;
        if (!WriteFile(target, data, static_cast<DWORD>(size), &count,
                       nullptr) || count == 0)
        {
            return false;
        }
        data += count;
        size -= count;
    }
    return true;
}

void append_le(std::vector<std::uint8_t>& frame,
               std::uint64_t value, std::size_t bytes)
{
    for (std::size_t index = 0; index < bytes; ++index)
    {
        frame.push_back(static_cast<std::uint8_t>(value >> (8 * index)));
    }
}

std::vector<std::uint8_t> frame(std::uint64_t session,
                                std::uint32_t length,
                                std::vector<std::uint8_t> payload)
{
    std::vector<std::uint8_t> result{'T', 'X', 'I', 'P'};
    append_le(result, 1, 4);
    append_le(result, session, 8);
    append_le(result, length, 4);
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
}

int echo_stdio()
{
    auto input = GetStdHandle(STD_INPUT_HANDLE);
    auto output = GetStdHandle(STD_OUTPUT_HANDLE);
    std::array<std::uint8_t, 20> header{};
    if (!read_exact(input, header.data(), header.size()))
    {
        return 1;
    }
    const std::uint32_t length =
        static_cast<std::uint32_t>(header[16]) |
        (static_cast<std::uint32_t>(header[17]) << 8) |
        (static_cast<std::uint32_t>(header[18]) << 16) |
        (static_cast<std::uint32_t>(header[19]) << 24);
    if (length > 1024 * 1024)
    {
        return 2;
    }
    std::vector<std::uint8_t> body(length);
    if (!read_exact(input, body.data(), body.size()) ||
        !write_exact(output, header.data(), header.size()) ||
        !write_exact(output, body.data(), body.size()))
    {
        return 3;
    }
    return 0;
}

int raw_client(const std::wstring& mode, const std::wstring& name)
{
    const auto path = L"\\\\.\\pipe\\tx-ipc-" + name;
    HANDLE handle = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < 200; ++attempt)
    {
        handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle != INVALID_HANDLE_VALUE)
        {
            break;
        }
        Sleep(10);
    }
    if (handle == INVALID_HANDLE_VALUE)
    {
        return 4;
    }
    bool written = false;
    if (mode == L"partial")
    {
        const auto data = frame(111, 5, {0x01, 0x02});
        written = write_exact(handle, data.data(), data.size());
    }
    else if (mode == L"oversize")
    {
        const auto data = frame(111, 1000, {});
        written = write_exact(handle, data.data(), data.size());
    }
    else if (mode == L"restart")
    {
        const auto first = frame(111, 1, {0x01});
        const auto second = frame(222, 1, {0x02});
        written = write_exact(handle, first.data(), first.size()) &&
            write_exact(handle, second.data(), second.size());
    }
    else if (mode == L"split")
    {
        const auto data = frame(333, 1, {0x03});
        written = write_exact(handle, data.data(), 9);
        Sleep(50);
        written = written && write_exact(handle,
            data.data() + 9, data.size() - 9);
    }
    CloseHandle(handle);
    return written ? 0 : 5;
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    if (argc == 2 && std::wstring(argv[1]) == L"echo")
    {
        return echo_stdio();
    }
    if (argc == 3)
    {
        return raw_client(argv[1], argv[2]);
    }
    return 6;
}
