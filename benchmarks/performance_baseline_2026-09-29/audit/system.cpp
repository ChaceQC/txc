#include <windows.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

using clock_type = std::chrono::steady_clock;

template<class Operation>
void measure(const char* name, Operation operation)
{
    const auto start = clock_type::now();
    const auto checksum = operation();
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        clock_type::now() - start).count();
    std::cout << name << '\n' << elapsed << '\n' << checksum << '\n';
}

std::int64_t file_stream_rw()
{
    const auto path = "tx_build/perf_audit_20260927/stream_cpp.bin";
    const std::string data = "0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef";
    std::array<char, 64> buffer{};
    std::int64_t checksum = 0;
    for (int i = 0; i < 200; ++i)
    {
        auto* output = std::fopen(path, "wb");
        if (!output || std::fwrite(data.data(), 1, data.size(), output) != data.size() ||
            std::fclose(output) != 0)
        {
            throw std::runtime_error("write failed");
        }
        auto* input = std::fopen(path, "rb");
        if (!input)
        {
            throw std::runtime_error("read failed");
        }
        checksum += std::fread(buffer.data(), 1, buffer.size(), input);
        if (std::fclose(input) != 0)
        {
            throw std::runtime_error("close failed");
        }
    }
    return checksum;
}

std::int64_t process_spawn()
{
    const std::wstring command = L"tx_build/perf_audit_20260927/child.exe";
    std::int64_t checksum = 0;
    for (int i = 0; i < 50; ++i)
    {
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        std::wstring mutable_command = command;
        if (!CreateProcessW(nullptr, mutable_command.data(), nullptr, nullptr,
            FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process))
        {
            throw std::runtime_error("spawn failed");
        }
        if (WaitForSingleObject(process.hProcess, 5000) != WAIT_OBJECT_0)
        {
            throw std::runtime_error("wait failed");
        }
        DWORD code = 1;
        if (!GetExitCodeProcess(process.hProcess, &code))
        {
            throw std::runtime_error("status failed");
        }
        checksum += code == 0 ? 1 : 0;
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
    }
    return checksum;
}

int main()
{
    measure("file_stream_rw", file_stream_rw);
    measure("process_spawn", process_spawn);
}
