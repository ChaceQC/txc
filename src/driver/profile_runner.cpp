#include "driver/profile_runner.hpp"
#include "driver/child_process.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace tx
{
namespace
{

struct profile_workspace
{
    std::filesystem::path path;

    profile_workspace()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int index = 0; index < 100; ++index)
        {
            const auto candidate = std::filesystem::temp_directory_path() /
                ("txc_profile_" + std::to_string(GetCurrentProcessId()) + "_" +
                 std::to_string(stamp) + "_" + std::to_string(index));
            if (std::filesystem::create_directory(candidate))
            {
                path = candidate;
                return;
            }
        }
        throw std::runtime_error("无法创建性能分析临时工作区");
    }

    ~profile_workspace()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};

std::string read_report(const std::filesystem::path& path)
{
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error || size > 64 * 1024 * 1024)
    {
        throw std::runtime_error("性能报告不存在或超过 64 MiB");
    }
    std::ifstream input(path, std::ios::binary);
    std::string result(static_cast<std::size_t>(size), '\0');
    if (!input.read(result.data(), static_cast<std::streamsize>(size)))
    {
        throw std::runtime_error("性能报告读取失败");
    }
    return result;
}

} // namespace

int run_profile(const std::filesystem::path& source, const profile_options& options,
    const std::function<int(const std::filesystem::path&, const std::filesystem::path&, int)>& compile)
{
    namespace fs = std::filesystem;
    const auto output = fs::absolute(options.output);
    if (fs::exists(output) && fs::equivalent(source, output))
    {
        throw std::runtime_error("性能报告不能覆盖源码文件");
    }
    profile_workspace workspace;
    const auto baseline = workspace.path / "baseline.exe";
    const auto profiled = workspace.path / "profiled.exe";
    if (compile(source, baseline, 0) || compile(source, profiled, options.interval_ms))
    {
        return 1;
    }
    std::ostringstream report;
    report << "{\"version\":1,\"environment\":{\"platform\":\"windows-x64\","
           << "\"optimization\":\"clang -O3\",\"cpu_count\":" << std::thread::hardware_concurrency()
           << "},\"warmup\":" << options.warmup << ",\"samples\":" << options.samples
           << ",\"interval_ms\":" << options.interval_ms << ",\"runs\":[";
    bool success = true;
    bool first = true;
    for (int index = 0; index < options.warmup + options.samples; ++index)
    {
        for (const bool instrumented : {false, true})
        {
            const auto directory = workspace.path /
                (std::to_string(index) + (instrumented ? "_profiled" : "_baseline"));
            fs::create_directory(directory);
            const auto result = run_child_process(instrumented ? profiled : baseline,
                                                  directory, options.timeout_ms);
            success &= !result.timed_out && result.exit_code == 0;
            std::string profile_json;
            if (instrumented && !result.timed_out && !result.exit_code)
            {
                profile_json = read_report(directory / "profile.json");
            }
            if (index < options.warmup && success)
            {
                continue;
            }
            report << (first ? "" : ",") << "{\"index\":" << index - options.warmup
                   << ",\"instrumented\":" << (instrumented ? "true" : "false")
                   << ",\"duration_ms\":" << result.duration_ms
                   << ",\"exit_code\":" << result.exit_code
                   << ",\"timeout\":" << (result.timed_out ? "true" : "false")
                   << ",\"output\":" << json_quote(result.output)
                   << ",\"profile_json\":" << json_quote(profile_json) << '}';
            first = false;
        }
        if (!success)
        {
            break;
        }
    }
    report << "],\"success\":" << (success ? "true" : "false") << "}\n";
    fs::create_directories(output.parent_path());
    std::ofstream destination(output, std::ios::binary);
    destination << report.str();
    destination.close();
    if (!destination)
    {
        throw std::runtime_error("无法写入性能分析汇总");
    }
    std::cout << (success ? "性能分析完成" : "性能分析程序失败") << '\n';
    return success ? 0 : 1;
}

} // namespace tx
