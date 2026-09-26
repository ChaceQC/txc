#include "driver/test_runner.hpp"

#include "common/common.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace tx
{
namespace
{

namespace fs = std::filesystem;

std::string path_text(const fs::path& path)
{
    const auto bytes = path.generic_u8string();
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

std::string json_quote(const std::string& value)
{
    constexpr char hex[] = "0123456789abcdef";
    std::string result = "\"";
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        const auto byte = static_cast<unsigned char>(value[index]);
        if (byte == '"' || byte == '\\')
        {
            result += '\\';
            result += static_cast<char>(byte);
        }
        else if (byte < 0x20)
        {
            result += "\\u00";
            result += hex[byte >> 4];
            result += hex[byte & 15];
        }
        else if (byte >= 0x80)
        {
            const std::size_t length = byte >= 0xc2 && byte <= 0xdf ? 2 :
                byte >= 0xe0 && byte <= 0xef ? 3 :
                byte >= 0xf0 && byte <= 0xf4 ? 4 : 0;
            std::uint32_t codepoint = byte & (length == 2 ? 0x1f :
                length == 3 ? 0x0f : 0x07);
            bool valid = length != 0 && index + length <= value.size();
            for (std::size_t part = 1; valid && part < length; ++part)
            {
                const auto continuation = static_cast<unsigned char>(
                    value[index + part]);
                valid = (continuation & 0xc0) == 0x80;
                codepoint = (codepoint << 6) | (continuation & 0x3f);
            }
            valid = valid && codepoint >= (length == 2 ? 0x80u :
                length == 3 ? 0x800u : 0x10000u) &&
                codepoint <= 0x10ffffu &&
                (codepoint < 0xd800u || codepoint > 0xdfffu);
            if (valid)
            {
                result.append(value, index, length);
                index += length - 1;
            }
            else
            {
                result += "\\ufffd";
            }
        }
        else
        {
            result += static_cast<char>(byte);
        }
    }
    return result + '"';
}

bool windows_crash_status(std::uint32_t code)
{
    // 只识别常见 SEH 退出状态；普通非零 main 返回码仍属于测试失败。
    switch (code)
    {
    case 0x80000003u: // breakpoint
    case 0xc0000005u: // access violation
    case 0xc000001du: // illegal instruction
    case 0xc0000094u: // integer divide by zero
    case 0xc0000095u: // integer overflow
    case 0xc00000fdu: // stack overflow
    case 0xc0000374u: // heap corruption
    case 0xc0000409u: // fail fast
        return true;
    default:
        return false;
    }
}

struct temporary_root
{
    fs::path path;

    explicit temporary_root()
    {
        const auto stamp = std::chrono::steady_clock::now()
            .time_since_epoch().count();
        for (int suffix = 0; suffix < 100; ++suffix)
        {
            auto candidate = fs::temp_directory_path() /
                ("txc_test_" + std::to_string(GetCurrentProcessId()) + "_" +
                 std::to_string(stamp) + "_" + std::to_string(suffix));
            std::error_code error;
            if (fs::create_directory(candidate, error))
            {
                path = std::move(candidate);
                return;
            }
            if (error && error != std::errc::file_exists)
            {
                throw fs::filesystem_error("无法创建测试工作区", candidate, error);
            }
        }
        throw std::runtime_error("无法取得测试工作区名称");
    }

    ~temporary_root()
    {
        if (!path.empty())
        {
            std::error_code ignored;
            fs::remove_all(path, ignored);
        }
    }
};

struct test_case
{
    fs::path source;
    std::string name;
    std::string status;
    std::string output;
    std::uint32_t exit_code = 0;

    test_case(fs::path selected_source, std::string selected_name)
        : source(std::move(selected_source)), name(std::move(selected_name))
    {
    }
};

std::vector<test_case> discover(const fs::path& source,
                                const std::optional<fs::path>& selected)
{
    std::vector<test_case> cases;
    if (fs::is_regular_file(source))
    {
        if (source.extension() != ".tx" || selected)
        {
            throw std::runtime_error("测试输入需要 .tx 文件，文件模式不能使用 --case");
        }
        cases.push_back({fs::canonical(source), path_text(source.filename())});
        return cases;
    }
    if (!fs::is_directory(source))
    {
        throw std::runtime_error("测试路径不存在或不是目录：" + path_text(source));
    }
    for (const auto& entry : fs::recursive_directory_iterator(source))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }
        const auto filename = entry.path().filename().string();
        if (!filename.ends_with("_test.tx"))
        {
            continue;
        }
        const auto relative = entry.path().lexically_relative(source);
        if (selected && relative.lexically_normal() != selected->lexically_normal())
        {
            continue;
        }
        cases.push_back({fs::canonical(entry.path()), path_text(relative)});
    }
    std::sort(cases.begin(), cases.end(),
        [](const test_case& left, const test_case& right)
        {
            return left.name < right.name;
        });
    if (cases.empty())
    {
        throw std::runtime_error(selected ? "找不到指定测试项" :
            "目录中没有 *_test.tx 测试项");
    }
    return cases;
}

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

void run_child(test_case& item, const fs::path& executable)
{
    SECURITY_ATTRIBUTES attributes{sizeof(attributes), nullptr, TRUE};
    windows_handle reader;
    windows_handle writer;
    if (!CreatePipe(&reader.value, &writer.value, &attributes, 0) ||
        !SetHandleInformation(reader.value, HANDLE_FLAG_INHERIT, 0))
    {
        throw std::runtime_error("无法建立测试输出管道");
    }
    windows_handle input{CreateFileW(L"NUL", GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, &attributes, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (input.value == INVALID_HANDLE_VALUE)
    {
        throw std::runtime_error("无法准备测试标准输入");
    }
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input.value;
    startup.hStdOutput = writer.value;
    startup.hStdError = writer.value;
    PROCESS_INFORMATION process{};
    std::wstring command = L"\"" + executable.wstring() + L"\"";
    const auto working_directory = item.source.parent_path().wstring();
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr,
                        TRUE, CREATE_NO_WINDOW, nullptr,
                        working_directory.c_str(), &startup, &process))
    {
        throw std::runtime_error("无法启动测试程序：" + item.name);
    }
    windows_handle process_handle{process.hProcess};
    windows_handle thread_handle{process.hThread};
    CloseHandle(writer.value);
    writer.value = INVALID_HANDLE_VALUE;
    char buffer[4096];
    DWORD count = 0;
    constexpr std::size_t output_limit = 4 * 1024 * 1024;
    while (ReadFile(reader.value, buffer, sizeof(buffer), &count, nullptr) &&
           count != 0)
    {
        if (item.output.size() < output_limit)
        {
            const auto remaining = output_limit - item.output.size();
            item.output.append(buffer, std::min<std::size_t>(count, remaining));
        }
    }
    if (item.output.size() == output_limit)
    {
        item.output += "\n[测试输出已截断]\n";
    }
    WaitForSingleObject(process_handle.value, INFINITE);
    DWORD code = 0;
    if (!GetExitCodeProcess(process_handle.value, &code))
    {
        throw std::runtime_error("无法读取测试退出码：" + item.name);
    }
    item.exit_code = code;
    item.status = code == 0 ? "passed" :
        windows_crash_status(code) ? "crashed" : "failed";
}

void write_report(const std::vector<test_case>& cases, bool json_format)
{
    int passed = 0;
    int failed = 0;
    int crashed = 0;
    int compile_errors = 0;
    for (const auto& item : cases)
    {
        passed += item.status == "passed";
        failed += item.status == "failed";
        crashed += item.status == "crashed";
        compile_errors += item.status == "compile_error";
    }
    if (json_format)
    {
        std::cout << "{\"summary\":{\"passed\":" << passed
                  << ",\"failed\":" << failed << ",\"crashed\":" << crashed
                  << ",\"compile_errors\":" << compile_errors
                  << "},\"cases\":[";
        for (std::size_t index = 0; index < cases.size(); ++index)
        {
            const auto& item = cases[index];
            std::cout << (index ? "," : "") << "{\"name\":"
                      << json_quote(item.name) << ",\"status\":"
                      << json_quote(item.status) << ",\"exit_code\":"
                      << item.exit_code << ",\"output\":"
                      << json_quote(item.output) << '}';
        }
        std::cout << "]}\n";
        return;
    }
    for (const auto& item : cases)
    {
        std::cout << '[' << item.status << "] " << item.name << '\n';
        if (!item.output.empty() && item.status != "passed")
        {
            std::cout << item.output;
            if (item.output.back() != '\n')
            {
                std::cout << '\n';
            }
        }
    }
    std::cout << "汇总：通过 " << passed << "，失败 " << failed
              << "，崩溃 " << crashed << "，编译错误 "
              << compile_errors << '\n';
}

} // namespace

int run_test_suite(const fs::path& source,
                   const std::optional<fs::path>& selected,
                   bool json_format,
                   const std::function<int(const fs::path&,
                                           const fs::path&)>& compile)
{
    auto cases = discover(source, selected);
    temporary_root root;
    for (std::size_t index = 0; index < cases.size(); ++index)
    {
        auto& item = cases[index];
        const auto case_dir = root.path / std::to_string(index);
        fs::create_directory(case_dir);
        const auto executable = case_dir / "test.exe";
        try
        {
            if (compile(item.source, executable) != 0)
            {
                item.status = "compile_error";
                item.output = "LLVM 后端编译失败";
                continue;
            }
        }
        catch (const compile_error& error)
        {
            const auto position = error.position();
            item.status = "compile_error";
            item.output = position.file + ':' +
                std::to_string(position.line) + ':' +
                std::to_string(position.column) + ": 错误：" + error.what();
            continue;
        }
        catch (const std::exception& error)
        {
            item.status = "compile_error";
            item.output = error.what();
            continue;
        }
        try
        {
            run_child(item, executable);
        }
        catch (const std::exception& error)
        {
            item.status = "crashed";
            item.output = error.what();
        }
    }
    write_report(cases, json_format);
    return std::all_of(cases.begin(), cases.end(),
        [](const test_case& item) { return item.status == "passed"; }) ? 0 : 1;
}

} // namespace tx
