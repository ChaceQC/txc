#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/task_executor.hpp"
#include "backend/cpp/task_runtime_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/filesystem_internal.hpp"
#include "stdlib/filesystem_extended.hpp"
#include "stdlib/process_internal.hpp"
#include "stdlib/task.hpp"

#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>

namespace
{

using namespace tx_generated;

struct file_request
{
    std::shared_ptr<task_scope_state> scope;
    std::shared_ptr<task_state> child;
    std::shared_ptr<cancellation_state> token;
    std::string path;
    std::string result_name;
    std::int64_t offset = 0;
    std::vector<std::uint8_t> data;
    bool writing = false;
    bool created = false;
    bool cancelled = false;
    std::size_t transferred = 0;
    std::string error;

    bool should_cancel()
    {
        return task_cancelled(*scope) || !process_detail::cancellation_reason(token).empty();
    }

    void open_and_transfer()
    {
        process_detail::native_handle file;
        if (writing)
        {
            file.reset(::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NONBLOCK, 0666));
            created = file.valid();
            if (!file.valid() && errno == EEXIST)
            {
                file.reset(::open(path.c_str(), O_WRONLY | O_CLOEXEC | O_NONBLOCK));
            }
        }
        else
        {
            file.reset(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK));
        }
        struct stat information{};
        if (!file.valid() || ::fstat(file.get(), &information) < 0 || !S_ISREG(information.st_mode))
        {
            error = "file_open_failed";
            return;
        }
        // 与 Windows 契约一致：打开/创建先于取消提交，created 反映真实外部效果。
        if (should_cancel())
        {
            cancelled = true;
            return;
        }
        // 普通文件由有界工作池执行 pread/pwrite；取消保留已完成块的实际前缀。
        while (transferred < data.size())
        {
            if (should_cancel())
            {
                cancelled = true;
                break;
            }
            const auto count = std::min<std::size_t>(65536, data.size() - transferred);
            const auto position = static_cast<off_t>(offset + transferred);
            const auto result = writing
                ? ::pwrite(file.get(), data.data() + transferred, count, position)
                : ::pread(file.get(), data.data() + transferred, count, position);
            if (result < 0 && errno == EINTR)
            {
                continue;
            }
            if (result < 0)
            {
                error = writing ? "file_write_failed" : "file_read_failed";
                break;
            }
            if (result == 0)
            {
                if (writing)
                {
                    error = "file_write_failed";
                }
                break;
            }
            transferred += static_cast<std::size_t>(result);
            if (!writing && static_cast<std::size_t>(result) < count)
            {
                break;
            }
        }
    }

    void run() noexcept
    {
        try
        {
            open_and_transfer();
            struct_fields fields(4);
            if (writing)
            {
                fields[0] = {"written", static_cast<std::int64_t>(transferred)};
                fields[1] = {"cancelled", cancelled};
                fields[2] = {"created", created};
            }
            else
            {
                const bool eof = error.empty() && !cancelled && transferred < data.size();
                data.resize(transferred);
                fields[0] = {"data", make_bytes(std::move(data))};
                fields[1] = {"eof", eof};
                fields[2] = {"cancelled", cancelled};
            }
            fields[3] = {"error_code", error};
            complete_task(child, std::any(dynamic_struct(dynamic_struct_data{
                result_name, writing ? "write_result" : "read_result", std::move(fields)})), {});
        }
        catch (...)
        {
            complete_task(child, {}, {tx::error_kind::runtime, "allocation_failed", "异步文件结果分配失败", {}});
        }
    }
};

void check_request(std::int64_t offset, std::int64_t size)
{
    if (offset < 0 || size < 0 || size > 16 * 1024 * 1024 ||
        offset > std::numeric_limits<std::int64_t>::max() - size)
    {
        throw runtime_failure({tx::error_kind::runtime, "out_of_range", "异步文件偏移或单次传输长度无效"});
    }
}

std::shared_ptr<file_request> prepare(const void* path, std::int64_t offset,
    const void* token, const char* result_name)
{
    auto request = std::make_shared<file_request>();
    request->scope = current_task_scope();
    if (!request->scope || !token)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_state", "异步文件操作需要活动作用域和取消令牌"});
    }
    request->token = std::any_cast<const cancel_token&>(*static_cast<const std::any*>(token)).state;
    if (!request->token)
    {
        throw runtime_failure({tx::error_kind::runtime, "invalid_state", "取消令牌已失效"});
    }
    request->path = detail::checked_path(detail::text_value(path)).string();
    request->offset = offset;
    request->result_name = result_name;
    return request;
}

void* schedule(const std::shared_ptr<file_request>& request, const char* task_name)
{
    request->child = reserve_task(request->scope);
    try
    {
        enqueue_task([request]
        {
            request->run();
        });
    }
    catch (...)
    {
        discard_task(request->child);
        throw;
    }
    return detail::make_handle<std::any>(task_handle{request->child, 5, task_name});
}

} // namespace

extern "C" int txrt_async_file_read_at(const void* path, std::int64_t offset,
    std::int64_t max_bytes, const void* token, const char* task_name,
    const char* result_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        check_request(offset, max_bytes);
        auto request = prepare(path, offset, token, result_name);
        request->data.resize(static_cast<std::size_t>(max_bytes));
        *result = schedule(request, task_name);
    });
}

extern "C" int txrt_async_file_write_at(const void* path, std::int64_t offset,
    const void* data, const void* token, const char* task_name,
    const char* result_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        const auto& bytes = tx_generated::bytes_of(*static_cast<const std::any*>(data));
        check_request(offset, static_cast<std::int64_t>(bytes->size()));
        auto request = prepare(path, offset, token, result_name);
        request->writing = true;
        request->data = *bytes;
        *result = schedule(request, task_name);
    });
}
