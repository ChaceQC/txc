#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <array>
#include <string>

namespace
{

bool environment_equals(const wchar_t* name, const std::wstring& expected)
{
    std::array<wchar_t, 256> value{};
    const auto size = GetEnvironmentVariableW(name, value.data(), value.size());
    return size < value.size() && std::wstring(value.data(), size) == expected;
}

bool write_all(HANDLE output, const char* bytes, DWORD size)
{
    while (size > 0)
    {
        DWORD written = 0;
        if (!WriteFile(output, bytes, size, &written, nullptr) || written == 0)
        {
            return false;
        }
        bytes += written;
        size -= written;
    }
    return true;
}

int argument_check(int argc, wchar_t** argv)
{
    const std::array<std::wstring, 7> expected = {
        L"", L"中文 空格", L"quote\"here", L"\\\\", L"tail\\", L"x\\\"y", L"&|><^%PATH%"};
    if (argc != static_cast<int>(expected.size()) + 2)
    {
        return 10;
    }
    for (std::size_t index = 0; index < expected.size(); ++index)
    {
        if (argv[index + 2] != expected[index])
        {
            return 11;
        }
    }
    std::array<wchar_t, 32768> cwd{};
    GetCurrentDirectoryW(cwd.size(), cwd.data());
    return std::wstring(cwd.data()).ends_with(L"子 目录") &&
        environment_equals(L"TX_PROCESS_TEST", L"覆盖=值") &&
        environment_equals(L"TX_PROCESS_PARENT", L"") ? 0 : 12;
}

int pump(bool flood)
{
    const auto input = GetStdHandle(STD_INPUT_HANDLE);
    const auto output = GetStdHandle(STD_OUTPUT_HANDLE);
    const auto error = GetStdHandle(STD_ERROR_HANDLE);
    std::array<char, 4096> buffer{};
    if (flood)
    {
        buffer.fill('o');
        for (int index = 0; index < 64; ++index)
        {
            if (!write_all(output, buffer.data(), buffer.size()))
            {
                return 20;
            }
            buffer.fill('e');
            if (!write_all(error, buffer.data(), buffer.size()))
            {
                return 21;
            }
            buffer.fill('o');
        }
    }
    DWORD received = 0;
    while (ReadFile(input, buffer.data(), buffer.size(), &received, nullptr) && received)
    {
        if (!write_all(output, buffer.data(), received))
        {
            return 22;
        }
    }
    return 0;
}

BOOL WINAPI on_control(DWORD event)
{
    if (event == CTRL_BREAK_EVENT)
    {
        ExitProcess(23);
    }
    return FALSE;
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2)
    {
        return 1;
    }
    const std::wstring mode(argv[1]);
    if (mode == L"arguments")
    {
        return argument_check(argc, argv);
    }
    if (mode == L"inherit_environment")
    {
        return environment_equals(L"TX_PROCESS_PARENT", L"parent-only") &&
            environment_equals(L"TX_PROCESS_TEST", L"child-only") ? 0 : 27;
    }
    if (mode == L"exit")
    {
        return 37;
    }
    if (mode == L"check_handle" && argc == 3)
    {
        const auto handle = reinterpret_cast<HANDLE>(std::stoull(argv[2]));
        DWORD flags = 0;
        return GetHandleInformation(handle, &flags) ? 24 : 0;
    }
    if (mode == L"control")
    {
        if (!SetConsoleCtrlHandler(on_control, TRUE) ||
            !write_all(GetStdHandle(STD_OUTPUT_HANDLE), "ready", 5))
        {
            return 25;
        }
        Sleep(30000);
        return 26;
    }
    if (mode == L"sleep")
    {
        Sleep(2000);
        return 0;
    }
    if (mode == L"echo" || mode == L"flood")
    {
        return pump(mode == L"flood");
    }
    if (mode == L"binary")
    {
        const char data[] = {'\0', '\1', static_cast<char>(0xff)};
        return write_all(GetStdHandle(STD_OUTPUT_HANDLE), data, sizeof(data)) ? 0 : 23;
    }
    if (mode == L"close_input")
    {
        CloseHandle(GetStdHandle(STD_INPUT_HANDLE));
        Sleep(100);
        return 0;
    }
    return 2;
}
