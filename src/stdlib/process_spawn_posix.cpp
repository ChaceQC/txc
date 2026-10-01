#include "stdlib/process_internal.hpp"

#include <cerrno>
#include <fcntl.h>
#include <spawn.h>

namespace tx_generated::process_detail
{

native_handle prepare_stream(const std::string& mode, unsigned index,
    process_pipe& parent)
{
    if (mode == "pipe")
    {
        int descriptors[2];
        if (::pipe2(descriptors, O_CLOEXEC) < 0)
        {
            fail("spawn_failed", "创建子进程管道失败", errno);
        }
        native_handle read_end(descriptors[0]), write_end(descriptors[1]);
        parent = std::make_shared<process_pipe_state>();
        parent->readable = index != 0;
        parent->handle = index == 0 ? std::move(write_end) : std::move(read_end);
        if (::fcntl(parent->handle.get(), F_SETFL, O_NONBLOCK) < 0)
        {
            fail("spawn_failed", "设置子进程管道失败", errno);
        }
        return index == 0 ? std::move(read_end) : std::move(write_end);
    }
    if (mode == "inherit")
    {
        const int copy = ::fcntl(static_cast<int>(index), F_DUPFD_CLOEXEC, 3);
        if (copy >= 0)
        {
            return native_handle(copy);
        }
        if (errno != EBADF)
        {
            fail("spawn_failed", "复制标准流失败", errno);
        }
    }
    else if (mode != "null")
    {
        fail("invalid_argument", "标准流策略必须为 pipe、inherit 或 null");
    }
    native_handle result(::open("/dev/null", O_CLOEXEC | (index == 0 ? O_RDONLY : O_WRONLY)));
    if (!result.valid())
    {
        fail("spawn_failed", "打开空标准流失败", errno);
    }
    return result;
}

} // namespace tx_generated::process_detail

namespace tx_generated
{
namespace
{

void check_spawn(int result)
{
    if (result)
    {
        process_detail::fail("spawn_failed", "启动子进程失败", result);
    }
}

struct spawn_actions
{
    posix_spawn_file_actions_t value;
    spawn_actions()
    {
        check_spawn(posix_spawn_file_actions_init(&value));
    }
    ~spawn_actions()
    {
        posix_spawn_file_actions_destroy(&value);
    }
};

struct spawn_attributes
{
    posix_spawnattr_t value;
    spawn_attributes()
    {
        check_spawn(posix_spawnattr_init(&value));
    }
    ~spawn_attributes()
    {
        posix_spawnattr_destroy(&value);
    }
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
    spawn_actions actions;
    spawn_attributes attributes;
    for (unsigned index = 0; index < streams.size(); ++index)
    {
        // fd 从 3 起复制，避免父进程关闭标准流时 dup2 顺序覆盖另一路。
        const int duplicate = ::fcntl(streams[index].get(), F_DUPFD_CLOEXEC, 3);
        if (duplicate < 0)
        {
            process_detail::fail("spawn_failed", "复制子进程标准流失败", errno);
        }
        streams[index].reset(duplicate);
        check_spawn(posix_spawn_file_actions_adddup2(&actions.value, duplicate, index));
    }
    check_spawn(posix_spawn_file_actions_addclosefrom_np(&actions.value, 3));
    if (!launch.cwd.empty())
    {
        check_spawn(posix_spawn_file_actions_addchdir_np(&actions.value, launch.cwd.c_str()));
    }
    if (options.new_process_group)
    {
        check_spawn(posix_spawnattr_setflags(&attributes.value, POSIX_SPAWN_SETPGROUP));
        check_spawn(posix_spawnattr_setpgroup(&attributes.value, 0));
    }
    std::vector<char*> arguments, environment;
    for (auto& item : launch.arguments)
    {
        arguments.push_back(item.data());
    }
    for (auto& item : launch.environment)
    {
        environment.push_back(item.data());
    }
    arguments.push_back(nullptr);
    environment.push_back(nullptr);
    pid_t pid;
    check_spawn(posix_spawn(&pid, launch.executable.c_str(), &actions.value,
        &attributes.value, arguments.data(), environment.data()));
    child->id = pid;
    child->group = options.new_process_group;
    return child;
}

} // namespace tx_generated
