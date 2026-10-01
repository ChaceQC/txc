#include "backend/cpp/runtime_diagnostics.hpp"

#include <cstdio>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <filesystem>
#include <fstream>
#endif

namespace tx_generated::detail
{
#ifdef _WIN32
namespace
{
bool needs_error_dialog()
{
    // 读取自身 PE，避免引入影响普通链接和 ThinLTO 的可变全局开关。
    const auto* base = reinterpret_cast<const unsigned char*>(GetModuleHandleW(nullptr));
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* pe = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (pe->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_WINDOWS_GUI)
    {
        return false;
    }
    const auto handle = GetStdHandle(STD_ERROR_HANDLE);
    SetLastError(ERROR_SUCCESS);
    return !handle || handle == INVALID_HANDLE_VALUE ||
        (GetFileType(handle) == FILE_TYPE_UNKNOWN && GetLastError() != ERROR_SUCCESS);
}

std::wstring wide_message(std::string_view message)
{
    const auto size = MultiByteToWideChar(CP_UTF8, 0, message.data(),
        static_cast<int>(message.size()), nullptr, 0);
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, message.data(), static_cast<int>(message.size()),
        result.data(), size);
    return result;
}

std::filesystem::path save_diagnostic(std::string_view message)
{
    for (const auto* variable : {L"LOCALAPPDATA", L"TEMP"})
    {
        try
        {
            const auto size = GetEnvironmentVariableW(variable, nullptr, 0);
            if (!size)
            {
                continue;
            }
            std::wstring directory(size, L'\0');
            if (GetEnvironmentVariableW(variable, directory.data(), size) != size - 1)
            {
                continue;
            }
            directory.resize(size - 1);
            const auto folder = std::filesystem::path(directory) / L"TX" / L"diagnostics";
            std::filesystem::create_directories(folder);
            FILETIME time{};
            GetSystemTimeAsFileTime(&time);
            const auto stamp = (static_cast<unsigned long long>(time.dwHighDateTime) << 32) |
                time.dwLowDateTime;
            const auto path = folder / (L"error-" + std::to_wstring(GetCurrentProcessId()) +
                L"-" + std::to_wstring(stamp) + L".log");
            std::ofstream output(path, std::ios::binary);
            output.write(message.data(), static_cast<std::streamsize>(message.size()));
            output.close();
            if (output)
            {
                return path;
            }
        }
        catch (...)
        {
            // 一个目录失败仍尝试下一位置；诊断不能覆盖最初的运行错误。
        }
    }
    return {};
}
}
#endif

void report_unhandled_error(std::string_view message) noexcept
{
#ifdef _WIN32
    if (needs_error_dialog())
    {
        try
        {
            const auto path = save_diagnostic(message);
            auto summary = wide_message(message.substr(0, message.find('\n')).substr(0, 512));
            summary += path.empty() ? L"\n诊断日志保存失败。" : L"\n诊断日志：\n" + path.wstring();
            if (path.empty())
            {
                OutputDebugStringW(wide_message(message).c_str());
            }
            MessageBoxW(nullptr, summary.c_str(), L"TX 运行错误", MB_OK | MB_ICONERROR);
        }
        catch (...)
        {
            MessageBoxW(nullptr, L"程序运行失败，诊断日志保存失败。", L"TX 运行错误",
                MB_OK | MB_ICONERROR);
        }
        return;
    }
#endif
    std::fwrite(message.data(), 1, message.size(), stderr);
    std::fflush(stderr);
}
}
