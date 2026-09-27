#include "stdlib/process_io.hpp"
#include "stdlib/process_io_internal.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/error.hpp"

#include <algorithm>

namespace tx_generated
{
namespace
{

using process_detail::pipe_operation;
using process_detail::steady_clock;

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
    if (limits.stop_action != "keep" && limits.stop_action != "terminate" &&
        limits.stop_action != "kill")
    {
        process_detail::fail("invalid_argument", "停止策略必须为 keep、terminate 或 kill");
    }
}

class run_session
{
public:
    run_session(process_child child, const process_limits& limits)
        : limits_(limits), operations_{
            pipe_operation(child->pipes[0]), pipe_operation(child->pipes[1]),
            pipe_operation(child->pipes[2])}
    {
        result_.child = std::move(child);
    }

    process_completed run(steady_clock::time_point limit,
        const std::shared_ptr<cancellation_state>& cancellation)
    {
        while (result_.reason == "completed")
        {
            start_io();
            bool progress = collect(1);
            progress = collect(2) || progress;
            progress = collect(0) || progress;
            const bool exited = process_detail::poll_exit(*result_.child);
            if (exited && done_[0] && done_[1] && done_[2])
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
            else if (steady_clock::now() >= limit)
            {
                result_.reason = "timeout";
            }
            else if (!progress)
            {
                wait_ready(limit);
            }
        }
        // 即便停止已与完成相遇，也先回收每个请求及它实际传输的前缀。
        for (auto& operation : operations_)
        {
            operation.cancel();
        }
        collect(1);
        collect(2);
        collect(0);
        apply_stop();
        result_.status = process_wait(result_.child, 0);
        result_.stdout_data = make_bytes(std::move(output_[0]));
        result_.stderr_data = make_bytes(std::move(output_[1]));
        return std::move(result_);
    }

private:
    void start_io()
    {
        for (unsigned index = 0; index < 3; ++index)
        {
            if (pending_[index] || done_[index])
            {
                continue;
            }
            if (index == 0)
            {
                const auto offset = static_cast<std::size_t>(result_.input_written);
                if (offset == limits_.input->size())
                {
                    done_[0] = true;
                    process_close_pipe(result_.child->pipes[0]);
                    continue;
                }
                const auto size = std::min<std::size_t>(block_size,
                    limits_.input->size() - offset);
                operations_[0].start(const_cast<std::uint8_t*>(
                    limits_.input->data() + offset), static_cast<DWORD>(size));
            }
            else
            {
                operations_[index].start(buffers_[index - 1].data(), block_size);
            }
            pending_[index] = true;
        }
    }

    bool collect(unsigned index)
    {
        auto& operation = operations_[index];
        if (!pending_[index] || !operation.poll())
        {
            return false;
        }
        pending_[index] = false;
        if (index == 0)
        {
            result_.input_written += operation.transferred;
        }
        else if (operation.transferred)
        {
            auto& output = output_[index - 1];
            const auto available = static_cast<std::size_t>(limits_.max_output_bytes)
                - output_[0].size() - output_[1].size();
            const auto size = std::min<std::size_t>(available, operation.transferred);
            output.insert(output.end(), buffers_[index - 1].begin(),
                buffers_[index - 1].begin() + size);
            if (size < operation.transferred && result_.reason == "completed")
            {
                result_.reason = "output_limit";
            }
        }
        if (process_detail::broken_pipe(operation.error) ||
            (index != 0 && operation.error == 0 && operation.transferred == 0))
        {
            done_[index] = true;
            if (index == 0)
            {
                process_close_pipe(result_.child->pipes[0]);
            }
            else
            {
                result_.child->pipes[index]->eof = true;
            }
        }
        else if (operation.error != 0 && operation.error != ERROR_OPERATION_ABORTED)
        {
            result_.reason = "pipe_failed";
        }
        return operation.transferred != 0 || done_[index];
    }

    void wait_ready(steady_clock::time_point limit)
    {
        std::array<HANDLE, 4> handles{};
        DWORD count = 0;
        for (const auto& operation : operations_)
        {
            if (operation.active())
            {
                handles[count++] = operation.event();
            }
        }
        if (result_.child->handle.valid())
        {
            handles[count++] = result_.child->handle.get();
        }
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            limit - steady_clock::now()).count();
        const auto timeout = static_cast<DWORD>(std::clamp<std::int64_t>(remaining, 1, 10));
        if (count == 0)
        {
            Sleep(timeout);
        }
        else if (WaitForMultipleObjects(count, handles.data(), FALSE, timeout) == WAIT_FAILED)
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

    static constexpr DWORD block_size = 16 * 1024;
    const process_limits& limits_;
    process_completed result_;
    std::array<std::vector<std::uint8_t>, 2> output_;
    std::array<std::array<std::uint8_t, block_size>, 2> buffers_{};
    std::array<pipe_operation, 3> operations_;
    std::array<bool, 3> pending_{};
    std::array<bool, 3> done_{};
};

} // namespace

process_completed process_run(const process_options& options,
    const process_limits& limits,
    const std::shared_ptr<cancellation_state>& cancellation)
{
    validate_run(options, limits);
    const auto reason = process_detail::cancellation_reason(cancellation);
    if (!reason.empty())
    {
        throw runtime_failure({tx::error_kind::cancelled, reason, "启动前进程操作已取消"});
    }
    const auto limit = process_detail::deadline(limits.timeout_ms);
    run_session session(process_spawn(options), limits);
    return session.run(limit, cancellation);
}

} // namespace tx_generated
