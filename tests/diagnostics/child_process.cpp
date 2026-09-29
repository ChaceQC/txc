#include "driver/child_process.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <windows.h>

int main(int argc, char* argv[])
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    const auto executable = std::filesystem::absolute(argv[0]);
    const auto directory = executable.parent_path();
    const std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "--crash")
    {
        RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, nullptr);
        return 1;
    }
    if (mode == "--marker")
    {
        Sleep(400);
        std::ofstream(directory / "orphan.txt") << "orphan";
        return 0;
    }
    if (mode == "--orphan")
    {
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION child{};
        auto command = L"\"" + executable.wstring() + L"\" --marker";
        if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr,
            FALSE, CREATE_NO_WINDOW, nullptr, directory.c_str(), &startup, &child))
        {
            return 1;
        }
        CloseHandle(child.hProcess);
        CloseHandle(child.hThread);
        return 0;
    }
    if (mode == "--flood")
    {
        const std::string chunk(4096, 'a');
        for (int index = 0; index < 1280; ++index)
        {
            std::fwrite(chunk.data(), 1, chunk.size(), stdout);
        }
        return 0;
    }
    const auto crash = tx::run_child_process(executable, directory, 2000, {L"--crash"});
    if (!tx::windows_crash_status(crash.exit_code) || crash.timed_out)
    {
        return 2;
    }
    const auto flood = tx::run_child_process(executable, directory, 5000, {L"--flood"});
    if (flood.exit_code || flood.timed_out || !flood.output_truncated || flood.output.size() != 4 * 1024 * 1024)
    {
        std::fprintf(stderr, "flood: exit=%lu timeout=%d truncated=%d bytes=%zu elapsed=%lld\n",
            static_cast<unsigned long>(flood.exit_code), flood.timed_out,
            flood.output_truncated, flood.output.size(), static_cast<long long>(flood.duration_ms));
        return 3;
    }
    const auto orphan = tx::run_child_process(executable, directory, 2000, {L"--orphan"});
    Sleep(700);
    if (orphan.exit_code || std::filesystem::exists(directory / "orphan.txt"))
    {
        return 4;
    }
    std::puts("原生崩溃、输出上限和后代清理通过");
    return 0;
}
