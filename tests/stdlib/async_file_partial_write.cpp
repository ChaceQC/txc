#include "../../src/backend/cpp/task_file_iocp.cpp"

#include "backend/cpp/value_format.hpp"
#include "stdlib/bytes.hpp"

#include <any>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{

bool verify_partial_result(DWORD bytes, DWORD error, bool cancelled,
                           const std::string& expected_error_code)
{
    auto child = std::make_shared<tx_generated::task_state>();
    write_operation operation("async_file", 0,
        tx_generated::make_bytes(std::vector<std::uint8_t>(12, 0x5a)));
    operation.child = child;
    operation.file = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!operation.file)
    {
        return false;
    }

    operation.complete(bytes, error);
    std::lock_guard lock(child->mutex);
    if (!child->completed || operation.file != INVALID_HANDLE_VALUE)
    {
        return false;
    }
    const auto* result_value = std::get_if<std::any>(&child->result);
    const auto* result = result_value
        ? std::any_cast<tx_generated::dynamic_struct>(result_value) : nullptr;
    if (!result || result->data->fields.size() != 4)
    {
        return false;
    }
    return std::any_cast<std::int64_t>(result->data->fields[0].value) == bytes &&
        std::any_cast<bool>(result->data->fields[1].value) == cancelled &&
        std::any_cast<bool>(result->data->fields[2].value) == false &&
        std::any_cast<std::string>(result->data->fields[3].value) ==
            expected_error_code;
}

} // namespace

int main()
{
    if (!verify_partial_result(5, ERROR_SUCCESS, false, "") ||
        !verify_partial_result(5, ERROR_OPERATION_ABORTED, true, "") ||
        !verify_partial_result(5, ERROR_DISK_FULL, false,
            "file_write_failed"))
    {
        std::cerr << "partial write result injection failed\n";
        return 1;
    }
    std::cout << "ASYNC_FILE_PARTIAL_WRITE_OK\n";
    return 0;
}
