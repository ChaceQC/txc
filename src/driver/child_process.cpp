#include "driver/child_process.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace tx
{
namespace
{

struct windows_handle
{
    HANDLE value = INVALID_HANDLE_VALUE;

    ~windows_handle()
    {
        if (value != INVALID_HANDLE_VALUE && value != nullptr)
        {
            CloseHandle(value);
        }
    }
};

struct process_attributes
{
    std::vector<unsigned char> storage;
    LPPROC_THREAD_ATTRIBUTE_LIST list = nullptr;

    process_attributes(HANDLE* handles, std::size_t count)
    {
        SIZE_T size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
        storage.resize(size);
        list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
        if (!InitializeProcThreadAttributeList(list, 1, 0, &size))
        {
            throw std::runtime_error("无法初始化子进程句柄列表");
        }
        if (!UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
            handles, count * sizeof(HANDLE), nullptr, nullptr))
        {
            DeleteProcThreadAttributeList(list);
            list = nullptr;
            throw std::runtime_error("无法限定子进程继承句柄");
        }
    }

    ~process_attributes()
    {
        if (list)
        {
            DeleteProcThreadAttributeList(list);
        }
    }
};

std::wstring quote_argument(const std::wstring& value)
{
    std::wstring result = L"\"";
    std::size_t slashes = 0;
    for (const auto character : value)
    {
        if (character == L'\\')
        {
            ++slashes;
            continue;
        }
        result.append(character == L'"' ? slashes * 2 + 1 : slashes, L'\\');
        result += character;
        slashes = 0;
    }
    result.append(slashes * 2, L'\\');
    return result + L'"';
}

void drain_output(HANDLE pipe, child_result& result)
{
    constexpr std::size_t output_limit = 4 * 1024 * 1024;
    // 每次只读固定批数，持续输出的子进程也不能饿死超时检查。
    for (int batch = 0; batch < 64; ++batch)
    {
        DWORD available = 0;
        if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr) || !available)
        {
            return;
        }
        char buffer[4096];
        DWORD count = 0;
        if (!ReadFile(pipe, buffer, std::min<DWORD>(available, sizeof(buffer)),
                      &count, nullptr) || !count)
        {
            return;
        }
        const auto kept = std::min<std::size_t>(count, output_limit - result.output.size());
        result.output.append(buffer, kept);
        result.output_truncated |= kept < count;
    }
}

} // namespace

child_result run_child_process(const std::filesystem::path& executable,
    const std::filesystem::path& working_directory, std::uint32_t timeout_ms,
    const std::vector<std::string>& arguments)
{
    SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
    windows_handle reader;
    windows_handle writer;
    if (!CreatePipe(&reader.value, &writer.value, &security, 65536) ||
        !SetHandleInformation(reader.value, HANDLE_FLAG_INHERIT, 0))
    {
        throw std::runtime_error("无法建立子进程输出管道");
    }
    windows_handle input{CreateFileW(L"NUL", GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, &security, OPEN_EXISTING, 0, nullptr)};
    windows_handle job{CreateJobObjectW(nullptr, nullptr)};
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (input.value == INVALID_HANDLE_VALUE || !job.value ||
        !SetInformationJobObject(job.value, JobObjectExtendedLimitInformation,
                                  &limits, sizeof(limits)))
    {
        throw std::runtime_error("无法准备子进程隔离工作区");
    }
    HANDLE inherited[] = {input.value, writer.value};
    process_attributes attributes(inherited, 2);
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = input.value;
    startup.StartupInfo.hStdOutput = writer.value;
    startup.StartupInfo.hStdError = writer.value;
    startup.lpAttributeList = attributes.list;
    PROCESS_INFORMATION process{};
    auto command = quote_argument(executable.wstring());
    for (const auto& argument : arguments)
    {
        command += L" " + quote_argument(std::filesystem::u8path(argument).wstring());
    }
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT,
        nullptr, working_directory.c_str(), &startup.StartupInfo, &process))
    {
        throw std::runtime_error("无法启动子进程");
    }
    windows_handle process_handle{process.hProcess};
    windows_handle thread_handle{process.hThread};
    if (!AssignProcessToJobObject(job.value, process.hProcess) ||
        ResumeThread(process.hThread) == static_cast<DWORD>(-1))
    {
        TerminateProcess(process.hProcess, 1);
        WaitForSingleObject(process.hProcess, INFINITE);
        throw std::runtime_error("无法启动受隔离的子进程");
    }
    CloseHandle(writer.value);
    writer.value = INVALID_HANDLE_VALUE;
    child_result result;
    const auto start = std::chrono::steady_clock::now();
    while (true)
    {
        drain_output(reader.value, result);
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (WaitForSingleObject(process.hProcess, 0) == WAIT_OBJECT_0)
        {
            break;
        }
        if (elapsed >= timeout_ms)
        {
            result.timed_out = true;
            TerminateJobObject(job.value, 1);
            WaitForSingleObject(process.hProcess, INFINITE);
            break;
        }
        WaitForSingleObject(process.hProcess, 2);
    }
    // 即使根进程正常返回，也不能让它的后代继续持有管道或修改已结束用例。
    TerminateJobObject(job.value, 1);
    drain_output(reader.value, result);
    DWORD code = 0;
    if (!GetExitCodeProcess(process.hProcess, &code))
    {
        throw std::runtime_error("无法读取子进程退出码");
    }
    result.exit_code = code;
    result.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    return result;
}

bool windows_crash_status(std::uint32_t code)
{
    switch (code)
    {
    case 0x80000003u:
    case 0xc0000005u:
    case 0xc000001du:
    case 0xc0000094u:
    case 0xc0000095u:
    case 0xc00000fdu:
    case 0xc0000374u:
    case 0xc0000409u:
        return true;
    default:
        return false;
    }
}

} // namespace tx
