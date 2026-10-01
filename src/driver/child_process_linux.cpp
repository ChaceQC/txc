#include "driver/child_process.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <stdexcept>
#include <system_error>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

namespace tx
{
namespace
{

struct descriptor
{
    int value = -1;
    ~descriptor()
    {
        if (value >= 0)
        {
            close(value);
        }
    }
};

void drain_output(int pipe, child_result& result)
{
    constexpr std::size_t limit = 4 * 1024 * 1024;
    for (int batch = 0; batch < 64; ++batch)
    {
        char buffer[4096];
        const auto count = read(pipe, buffer, sizeof(buffer));
        if (count <= 0)
        {
            return;
        }
        const auto kept = std::min<std::size_t>(count, limit - result.output.size());
        result.output.append(buffer, kept);
        result.output_truncated |= kept < static_cast<std::size_t>(count);
    }
}

} // namespace

child_result run_child_process(const std::filesystem::path& executable,
    const std::filesystem::path& working_directory, std::uint32_t timeout_ms,
    const std::vector<std::string>& arguments)
{
    int pipes[2];
    if (pipe2(pipes, O_CLOEXEC) != 0)
    {
        throw std::system_error(errno, std::generic_category(), "无法建立子进程输出管道");
    }
    descriptor reader{pipes[0]};
    descriptor writer{pipes[1]};
    descriptor input{open("/dev/null", O_RDONLY | O_CLOEXEC)};
    if (input.value < 0 || fcntl(reader.value, F_SETFL, O_NONBLOCK) < 0)
    {
        throw std::system_error(errno, std::generic_category(), "无法准备子进程输入输出");
    }
    std::vector<std::string> storage{std::filesystem::absolute(executable).string()};
    storage.insert(storage.end(), arguments.begin(), arguments.end());
    std::vector<char*> values;
    for (auto& value : storage)
    {
        values.push_back(value.data());
    }
    values.push_back(nullptr);
    const auto child = fork();
    if (child < 0)
    {
        throw std::system_error(errno, std::generic_category(), "无法启动子进程");
    }
    if (child == 0)
    {
        if (setpgid(0, 0) != 0 || chdir(working_directory.c_str()) != 0 ||
            dup2(input.value, STDIN_FILENO) < 0 || dup2(writer.value, STDOUT_FILENO) < 0 ||
            dup2(writer.value, STDERR_FILENO) < 0)
        {
            _exit(126);
        }
        execv(storage.front().c_str(), values.data());
        _exit(127);
    }
    setpgid(child, child);
    close(writer.value);
    writer.value = -1;
    struct child_owner
    {
        pid_t id;
        bool reaped = false;
        ~child_owner()
        {
            kill(-id, SIGKILL);
            if (!reaped)
            {
                while (waitpid(id, nullptr, 0) < 0 && errno == EINTR)
                {
                }
            }
        }
    } owner{child};
    child_result result;
    const auto start = std::chrono::steady_clock::now();
    int status = 0;
    while (true)
    {
        drain_output(reader.value, result);
        const auto waited = waitpid(child, &status, WNOHANG);
        if (waited == child)
        {
            owner.reaped = true;
            break;
        }
        if (waited < 0 && errno != EINTR)
        {
            throw std::system_error(errno, std::generic_category(), "无法等待子进程");
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= timeout_ms)
        {
            result.timed_out = true;
            kill(-child, SIGKILL);
            while (waitpid(child, &status, 0) < 0 && errno == EINTR)
            {
            }
            owner.reaped = true;
            break;
        }
        pollfd ready{reader.value, POLLIN, 0};
        poll(&ready, 1, 2);
    }
    kill(-child, SIGKILL);
    drain_output(reader.value, result);
    result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) :
        0x80000000u | static_cast<unsigned>(WTERMSIG(status));
    result.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    return result;
}

bool windows_crash_status(std::uint32_t code)
{
    return (code & 0x80000000u) != 0;
}

} // namespace tx
