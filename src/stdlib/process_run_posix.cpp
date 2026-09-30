#include "stdlib/process_internal.hpp"
#include "stdlib/process_io.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/error.hpp"

#include <algorithm>
#include <cerrno>
#include <poll.h>

namespace tx_generated
{
namespace
{

void validate_run(const process_options& options, const process_limits& limits)
{
    process_detail::validate_timeout(limits.timeout_ms);
    constexpr std::int64_t hard_limit = 64 * 1024 * 1024;
    if (!limits.input || limits.input->size() > hard_limit ||
        limits.max_output_bytes < 0 || limits.max_output_bytes > hard_limit)
    {
        process_detail::fail("invalid_argument", "进程输入及捕获上限必须在 0～64 MiB 内");
    }
    if (options.stdin_mode != "pipe" || options.stdout_mode != "pipe" ||
        options.stderr_mode != "pipe")
    {
        process_detail::fail("invalid_argument", "run 要求三路标准流均配置为 pipe");
    }
    if (limits.stop_action != "keep" && limits.stop_action != "terminate" && limits.stop_action != "kill")
    {
        process_detail::fail("invalid_argument", "停止策略必须为 keep、terminate 或 kill");
    }
}

class run_session
{
public:
    run_session(process_child child, const process_limits& limits) : limits_(limits)
    {
        result_.child = std::move(child);
    }

    process_completed run(process_detail::steady_clock::time_point limit,
        const std::shared_ptr<cancellation_state>& cancellation)
    {
        while (result_.reason == "completed")
        {
            transfer_output(1);
            transfer_output(2);
            transfer_input();
            if (process_detail::poll_exit(*result_.child) && done_[0] && done_[1] && done_[2])
            {
                break;
            }
            if (result_.reason != "completed")
            {
                break;
            }
            const auto reason = process_detail::cancellation_reason(cancellation);
            if (!reason.empty())
            {
                result_.reason = reason;
            }
            else if (process_detail::steady_clock::now() >= limit)
            {
                result_.reason = "timeout";
            }
            else
            {
                std::array<pollfd, 3> items;
                for (unsigned index = 0; index < 3; ++index)
                {
                    items[index] = {done_[index] ? -1 : result_.child->pipes[index]->handle.get(),
                        static_cast<short>(index == 0 ? POLLOUT : POLLIN), 0};
                }
                if (::poll(items.data(), items.size(), 1) < 0 && errno != EINTR)
                {
                    result_.reason = "pipe_failed";
                }
            }
        }
        apply_stop();
        result_.status = process_wait(result_.child, 0);
        result_.stdout_data = make_bytes(std::move(output_[0]));
        result_.stderr_data = make_bytes(std::move(output_[1]));
        return std::move(result_);
    }

private:
    void transfer_input()
    {
        if (done_[0])
        {
            return;
        }
        const auto offset = static_cast<std::size_t>(result_.input_written);
        if (offset == limits_.input->size())
        {
            done_[0] = true;
            process_close_pipe(result_.child->pipes[0]);
            return;
        }
        const auto size = process_detail::write_no_signal(result_.child->pipes[0]->handle.get(),
            limits_.input->data() + offset, std::min<std::size_t>(16384, limits_.input->size() - offset));
        if (size >= 0)
        {
            result_.input_written += size;
        }
        else if (errno == EPIPE)
        {
            done_[0] = true;
            process_close_pipe(result_.child->pipes[0]);
        }
        else if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            result_.reason = "pipe_failed";
        }
    }

    void transfer_output(unsigned index)
    {
        if (done_[index])
        {
            return;
        }
        std::array<std::uint8_t, 16384> buffer;
        const auto size = ::read(result_.child->pipes[index]->handle.get(), buffer.data(), buffer.size());
        if (size > 0)
        {
            const auto available = static_cast<std::size_t>(limits_.max_output_bytes)
                - output_[0].size() - output_[1].size();
            const auto accepted = std::min<std::size_t>(available, size);
            auto& output = output_[index - 1];
            output.insert(output.end(), buffer.begin(), buffer.begin() + accepted);
            if (accepted < static_cast<std::size_t>(size))
            {
                result_.reason = "output_limit";
            }
        }
        else if (size == 0)
        {
            done_[index] = true;
            result_.child->pipes[index]->eof = true;
        }
        else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
        {
            result_.reason = "pipe_failed";
        }
    }

    void apply_stop()
    {
        if (result_.reason == "completed" || limits_.stop_action == "keep")
        {
            return;
        }
        try
        {
            if (limits_.stop_action == "kill")
            {
                process_kill(result_.child);
                process_wait(result_.child, -1);
            }
            else
            {
                process_terminate(result_.child);
            }
        }
        catch (const runtime_failure&)
        {
            result_.reason = "terminate_failed";
        }
    }

    const process_limits& limits_;
    process_completed result_;
    std::array<std::vector<std::uint8_t>, 2> output_;
    std::array<bool, 3> done_{};
};

} // namespace

process_completed process_run(const process_options& options, const process_limits& limits,
    const std::shared_ptr<cancellation_state>& cancellation)
{
    validate_run(options, limits);
    const auto reason = process_detail::cancellation_reason(cancellation);
    if (!reason.empty())
    {
        throw runtime_failure({tx::error_kind::cancelled, reason, "启动前进程操作已取消"});
    }
    const auto limit = process_detail::deadline(limits.timeout_ms);
    return run_session(process_spawn(options), limits).run(limit, cancellation);
}

} // namespace tx_generated
