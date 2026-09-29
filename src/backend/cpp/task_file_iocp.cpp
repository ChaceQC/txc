#include "backend/cpp/task_iocp.hpp"

#include "backend/cpp/runtime_abi_internal.hpp"
#include "backend/cpp/task_runtime_internal.hpp"
#include "backend/cpp/value_format.hpp"
#include "stdlib/bytes.hpp"
#include "stdlib/encoding.hpp"

#include <algorithm>
#include <any>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace
{

constexpr std::int64_t maximum_transfer = 16 * 1024 * 1024;

void check_request(std::int64_t offset, std::int64_t size)
{
    if (offset < 0 || size < 0 || size > maximum_transfer ||
        offset > std::numeric_limits<std::int64_t>::max() - size)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "out_of_range", "异步文件偏移或单次传输长度无效"});
    }
}

std::shared_ptr<tx_generated::task_scope_state> require_scope()
{
    auto scope = tx_generated::current_task_scope();
    if (!scope)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_state", "异步文件操作需要活动的 task.scope"});
    }
    return scope;
}

std::shared_ptr<tx_generated::cancellation_state> require_token(
    const void* value)
{
    if (!value)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_argument", "异步文件操作缺少取消令牌"});
    }
    const auto& token = std::any_cast<const tx_generated::cancel_token&>(
        *static_cast<const std::any*>(value));
    if (!token.state)
    {
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "invalid_state", "取消令牌已失效"});
    }
    return token.state;
}

#ifdef _WIN32

using tx_generated::task_io_operation;

class file_operation : public task_io_operation
{
public:
    explicit file_operation(std::string name, std::int64_t offset)
        : result_name(std::move(name))
    {
        overlapped.Offset = static_cast<DWORD>(offset);
        overlapped.OffsetHigh = static_cast<DWORD>(
            static_cast<std::uint64_t>(offset) >> 32);
    }

    void finish_failure() noexcept
    {
        try
        {
            tx_generated::complete_task(child, {},
                {tx::error_kind::runtime, "allocation_failed",
                 "异步文件结果分配失败", {}});
        }
        catch (...)
        {
            // 完成状态分配失败属于进程级内存耗尽；不从 IOCP 线程抛异常。
        }
    }

    [[nodiscard]] virtual bool empty() const noexcept = 0;

    std::string result_name;
    std::string stage_code;
};

class read_operation final : public file_operation
{
public:
    read_operation(std::string name, std::int64_t offset,
                   std::size_t size)
        : file_operation(std::move(name), offset), buffer(size)
    {
    }

    DWORD begin() noexcept override
    {
        if (should_cancel())
        {
            return ERROR_OPERATION_ABORTED;
        }
        const auto started = ReadFile(file, buffer.data(),
            static_cast<DWORD>(buffer.size()), nullptr, &overlapped);
        return started ? ERROR_SUCCESS : GetLastError();
    }

    [[nodiscard]] bool empty() const noexcept override
    {
        return buffer.empty();
    }

    void complete(DWORD bytes, DWORD error) noexcept override
    {
        close_file();
        try
        {
            const auto count = std::min<std::size_t>(bytes, buffer.size());
            buffer.resize(count);
            const bool cancelled = error == ERROR_OPERATION_ABORTED;
            const bool eof = error == ERROR_HANDLE_EOF ||
                (error == ERROR_SUCCESS && count < requested);
            const auto code = cancelled || error == ERROR_SUCCESS ||
                error == ERROR_HANDLE_EOF ? std::string{} :
                stage_code.empty() ? std::string("file_read_failed") : stage_code;
            tx_generated::struct_fields fields(4);
            fields[0] = {"data", tx_generated::make_bytes(std::move(buffer))};
            fields[1] = {"eof", eof};
            fields[2] = {"cancelled", cancelled};
            fields[3] = {"error_code", code};
            tx_generated::complete_task(child,
                std::any(tx_generated::dynamic_struct(
                    tx_generated::dynamic_struct_data{result_name,
                        "read_result", std::move(fields)})), {});
        }
        catch (...)
        {
            finish_failure();
        }
    }

    std::vector<std::uint8_t> buffer;
    std::size_t requested = buffer.size();
};

class write_operation final : public file_operation
{
public:
    write_operation(std::string name, std::int64_t offset,
                    tx_generated::byte_value value)
        : file_operation(std::move(name), offset), data(std::move(value))
    {
    }

    DWORD begin() noexcept override
    {
        if (should_cancel())
        {
            return ERROR_OPERATION_ABORTED;
        }
        const auto started = WriteFile(file, data->data(),
            static_cast<DWORD>(data->size()), nullptr, &overlapped);
        return started ? ERROR_SUCCESS : GetLastError();
    }

    [[nodiscard]] bool empty() const noexcept override
    {
        return data->empty();
    }

    void complete(DWORD bytes, DWORD error) noexcept override
    {
        close_file();
        try
        {
            const bool cancelled = error == ERROR_OPERATION_ABORTED;
            const auto code = cancelled || error == ERROR_SUCCESS
                ? std::string{} : stage_code.empty()
                ? std::string("file_write_failed") : stage_code;
            tx_generated::struct_fields fields(4);
            fields[0] = {"written", static_cast<std::int64_t>(bytes)};
            fields[1] = {"cancelled", cancelled};
            fields[2] = {"created", created};
            fields[3] = {"error_code", code};
            tx_generated::complete_task(child,
                std::any(tx_generated::dynamic_struct(
                    tx_generated::dynamic_struct_data{result_name,
                        "write_result", std::move(fields)})), {});
        }
        catch (...)
        {
            finish_failure();
        }
    }

    tx_generated::byte_value data;
    bool created = false;
};

template<class operation_type>
void* schedule_file(std::shared_ptr<operation_type> operation,
                    const char* task_name, const char* path,
                    bool writing)
{
    operation->scope = require_scope();
    operation->child = tx_generated::reserve_task(operation->scope);
    try
    {
        const auto native = tx_generated::detail::utf8_to_wide(path);
        operation->file = CreateFileW(native.c_str(),
            writing ? GENERIC_WRITE : GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, writing ? OPEN_ALWAYS : OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
        if (operation->file == INVALID_HANDLE_VALUE)
        {
            operation->stage_code = "file_open_failed";
            operation->complete(0, GetLastError());
        }
        else
        {
            if constexpr (std::is_same_v<operation_type, write_operation>)
            {
                operation->created = GetLastError() != ERROR_ALREADY_EXISTS;
            }
            if (operation->empty())
            {
                operation->complete(0, operation->should_cancel()
                    ? ERROR_OPERATION_ABORTED : ERROR_SUCCESS);
            }
            else
            {
                tx_generated::submit_io_operation(operation);
            }
        }
    }
    catch (...)
    {
        tx_generated::discard_task(operation->child);
        throw;
    }
    return tx_generated::detail::make_handle<std::any>(
        tx_generated::task_handle{operation->child, 5, task_name});
}

#endif

} // namespace

extern "C" int txrt_async_file_read_at(const void* path,
    std::int64_t offset, std::int64_t max_bytes, const void* token,
    const char* task_name, const char* result_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        check_request(offset, max_bytes);
#ifdef _WIN32
        auto operation = std::make_shared<read_operation>(result_name, offset,
            static_cast<std::size_t>(max_bytes));
        operation->token = require_token(token);
        *result = schedule_file(std::move(operation), task_name,
            tx_generated::detail::text_value(path).c_str(), false);
#else
        (void)path;
        (void)token;
        (void)task_name;
        (void)result_name;
        (void)result;
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "iocp_unavailable", "当前平台不支持 Windows 异步文件 I/O"});
#endif
    });
}

extern "C" int txrt_async_file_write_at(const void* path,
    std::int64_t offset, const void* data, const void* token,
    const char* task_name, const char* result_name, void** result) noexcept
{
    return tx_generated::detail::invoke_checked([&]
    {
        const auto& bytes = tx_generated::bytes_of(
            *static_cast<const std::any*>(data));
        check_request(offset, static_cast<std::int64_t>(bytes->size()));
#ifdef _WIN32
        auto operation = std::make_shared<write_operation>(result_name, offset,
            bytes);
        operation->token = require_token(token);
        *result = schedule_file(std::move(operation), task_name,
            tx_generated::detail::text_value(path).c_str(), true);
#else
        (void)path;
        (void)token;
        (void)task_name;
        (void)result_name;
        (void)result;
        throw tx_generated::runtime_failure({tx::error_kind::runtime,
            "iocp_unavailable", "当前平台不支持 Windows 异步文件 I/O"});
#endif
    });
}
