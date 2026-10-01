#include "driver/test_runner.hpp"
#include "driver/child_process.hpp"

#include "common/common.hpp"

#include <algorithm>
#include <atomic>
#include <thread>
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
#include "common/platform.hpp"

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
                ("txc_test_" + std::to_string(tx::process_id()) + "_" +
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
    std::int64_t duration_ms = 0;
    bool output_truncated = false;
    fs::path executable;
    fs::path working_directory;

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

void write_report(const std::vector<test_case>& cases, bool json_format)
{
    int passed = 0;
    int failed = 0;
    int crashed = 0;
    int compile_errors = 0;
    int timeouts = 0;
    for (const auto& item : cases)
    {
        passed += item.status == "passed";
        failed += item.status == "failed";
        crashed += item.status == "crashed";
        compile_errors += item.status == "compile_error";
        timeouts += item.status == "timeout";
    }
    if (json_format)
    {
        std::cout << "{\"version\":2,\"summary\":{\"passed\":" << passed
                  << ",\"failed\":" << failed << ",\"crashed\":" << crashed
                  << ",\"compile_errors\":" << compile_errors
                  << ",\"timeouts\":" << timeouts
                  << "},\"cases\":[";
        for (std::size_t index = 0; index < cases.size(); ++index)
        {
            const auto& item = cases[index];
            std::cout << (index ? "," : "") << "{\"name\":"
                      << json_quote(item.name) << ",\"status\":"
                      << json_quote(item.status) << ",\"exit_code\":"
                      << item.exit_code << ",\"output\":"
                      << json_quote(item.output) << ",\"duration_ms\":" << item.duration_ms
                      << ",\"output_truncated\":" << (item.output_truncated ? "true" : "false") << '}';
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
              << compile_errors << "，超时 " << timeouts << '\n';
}

} // namespace

int run_test_suite(const fs::path& source,
                   const std::optional<fs::path>& selected,
                   const test_options& options,
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
        const auto executable = case_dir / tx::executable_name("test");
        item.executable = executable;
        item.working_directory = options.source_directory ? item.source.parent_path() : case_dir;
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
    }
    std::atomic_size_t cursor = 0;
    const auto worker = [&]
    {
        while (true)
        {
            const auto index = cursor.fetch_add(1);
            if (index >= cases.size())
            {
                return;
            }
            auto& item = cases[index];
            if (!item.status.empty())
            {
                continue;
            }
            try
            {
                auto result = run_child_process(item.executable, item.working_directory,
                                                options.timeout_ms);
                item.exit_code = result.exit_code;
                item.duration_ms = result.duration_ms;
                item.output_truncated = result.output_truncated;
                item.output = std::move(result.output);
                item.status = result.timed_out ? "timeout" :
                    result.exit_code == 0 ? "passed" :
                    windows_crash_status(result.exit_code) ? "crashed" : "failed";
            }
            catch (const std::exception& error)
            {
                item.status = "crashed";
                item.output = error.what();
            }
        }
    };
    {
        std::vector<std::jthread> workers;
        for (std::size_t index = 0; index < std::min<std::size_t>(options.jobs, cases.size()); ++index)
        {
            workers.emplace_back(worker);
        }
    }
    write_report(cases, options.json_format);
    return std::all_of(cases.begin(), cases.end(),
        [](const test_case& item)
        {
            return item.status == "passed";
        }) ? 0 : 1;
}

} // namespace tx
