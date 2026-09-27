#include "stdlib/process_internal.hpp"

#include <atomic>

namespace tx_generated::process_detail
{
namespace
{

native_handle pipe_pair(bool input, process_pipe& parent)
{
    // 序号只用于本进程管道命名，不保存 TX 值或控制用户进程生命周期。
    static std::atomic<std::uint64_t> next_pipe = 0;
    const auto name = L"\\\\.\\pipe\\tx-process-" +
        std::to_wstring(GetCurrentProcessId()) + L"-" +
        std::to_wstring(next_pipe.fetch_add(1));
    parent = std::make_shared<process_pipe_state>();
    parent->readable = !input;
    parent->handle.reset(CreateNamedPipeW(name.c_str(),
        (input ? PIPE_ACCESS_OUTBOUND : PIPE_ACCESS_INBOUND) |
            (input ? 0 : FILE_FLAG_OVERLAPPED) | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE | (input ? PIPE_NOWAIT : PIPE_WAIT) | PIPE_REJECT_REMOTE_CLIENTS,
        1, 64 * 1024, 64 * 1024, 0, nullptr));
    if (!parent->handle.valid())
    {
        fail("spawn_failed", "创建子进程管道失败", GetLastError());
    }
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    native_handle child(CreateFileW(name.c_str(), input ? GENERIC_READ : GENERIC_WRITE,
        0, &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!child.valid())
    {
        fail("spawn_failed", "连接子进程管道失败", GetLastError());
    }
    // 同一线程的客户端已连接；仍完成 server 侧的连接协议。
    native_handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!event.valid())
    {
        fail("spawn_failed", "创建管道连接事件失败", GetLastError());
    }
    OVERLAPPED operation{};
    operation.hEvent = event.get();
    if (!ConnectNamedPipe(parent->handle.get(), input ? nullptr : &operation))
    {
        const auto error = GetLastError();
        if (error != ERROR_PIPE_CONNECTED)
        {
            fail("spawn_failed", "完成管道连接失败", error);
        }
    }
    return child;
}

} // namespace

native_handle prepare_stream(const std::string& mode, unsigned index,
    process_pipe& parent)
{
    if (mode == "pipe")
    {
        return pipe_pair(index == 0, parent);
    }
    native_handle result;
    if (mode == "inherit")
    {
        const DWORD ids[] = {STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE};
        const auto source = GetStdHandle(ids[index]);
        if (source && source != INVALID_HANDLE_VALUE)
        {
            HANDLE copy = nullptr;
            if (!DuplicateHandle(GetCurrentProcess(), source, GetCurrentProcess(),
                    &copy, 0, TRUE, DUPLICATE_SAME_ACCESS))
            {
                fail("spawn_failed", "复制标准流句柄失败", GetLastError());
            }
            return native_handle(copy);
        }
    }
    else if (mode != "null")
    {
        fail("invalid_argument", "标准流策略必须为 pipe、inherit 或 null");
    }
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    result.reset(CreateFileW(L"NUL", index == 0 ? GENERIC_READ : GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, &security, OPEN_EXISTING, 0, nullptr));
    if (!result.valid())
    {
        fail("spawn_failed", "打开空标准流失败", GetLastError());
    }
    return result;
}

} // namespace tx_generated::process_detail

namespace tx_generated
{
namespace
{

class startup_attributes
{
public:
    explicit startup_attributes(const std::array<HANDLE, 3>& handles)
    {
        SIZE_T size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
        data_.resize(size);
        list_ = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(data_.data());
        if (!InitializeProcThreadAttributeList(list_, 1, 0, &size))
        {
            process_detail::fail("spawn_failed", "初始化进程句柄列表失败", GetLastError());
        }
        if (!UpdateProcThreadAttribute(list_, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                const_cast<HANDLE*>(handles.data()), sizeof(handles), nullptr, nullptr))
        {
            const auto error = GetLastError();
            DeleteProcThreadAttributeList(list_);
            process_detail::fail("spawn_failed", "设置进程句柄列表失败", error);
        }
    }
    ~startup_attributes()
    {
        DeleteProcThreadAttributeList(list_);
    }
    LPPROC_THREAD_ATTRIBUTE_LIST get() const noexcept
    {
        return list_;
    }
private:
    std::vector<unsigned char> data_;
    LPPROC_THREAD_ATTRIBUTE_LIST list_ = nullptr;
};

} // namespace

process_child process_spawn(const process_options& options)
{
    auto launch = process_detail::prepare_launch(options);
    auto child = std::make_shared<process_child_state>();
    std::array<process_detail::native_handle, 3> streams;
    streams[0] = process_detail::prepare_stream(options.stdin_mode, 0, child->pipes[0]);
    streams[1] = process_detail::prepare_stream(options.stdout_mode, 1, child->pipes[1]);
    streams[2] = process_detail::prepare_stream(options.stderr_mode, 2, child->pipes[2]);
    const std::array<HANDLE, 3> handles = {
        streams[0].get(), streams[1].get(), streams[2].get()};
    startup_attributes attributes(handles);
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.StartupInfo.wShowWindow = SW_HIDE;
    startup.StartupInfo.hStdInput = handles[0];
    startup.StartupInfo.hStdOutput = handles[1];
    startup.StartupInfo.hStdError = handles[2];
    startup.lpAttributeList = attributes.get();
    PROCESS_INFORMATION information{};
    const DWORD flags = EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT |
        (options.new_process_group ? CREATE_NEW_PROCESS_GROUP : 0);
    if (!CreateProcessW(launch.executable.c_str(), launch.command_line.data(),
            nullptr, nullptr, TRUE, flags, launch.environment.data(),
            launch.cwd.empty() ? nullptr : launch.cwd.c_str(),
            &startup.StartupInfo, &information))
    {
        process_detail::fail("spawn_failed", "启动子进程失败", GetLastError());
    }
    child->handle.reset(information.hProcess);
    process_detail::native_handle thread(information.hThread);
    child->id = information.dwProcessId;
    child->group = options.new_process_group;
    return child;
}

} // namespace tx_generated
