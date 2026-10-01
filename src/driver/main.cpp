#include "backend/llvm/codegen.hpp"
#include "common/platform.hpp"
#include "driver/native_toolchain.hpp"
#include "driver/compatibility.hpp"
#include "driver/module_loader.hpp"
#include "driver/test_runner.hpp"
#include "driver/profile_runner.hpp"
#include "frontend/resolver/module_resolver.hpp"
#include "frontend/sema/sema.hpp"

#include <chrono>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

namespace fs = std::filesystem;

std::string path_text(const fs::path& path)
{
    const auto utf8 = path.u8string();
    return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}

using tx::executable_path;
using tx::compile_llvm_native;

struct temporary_file
{
    fs::path path;

    ~temporary_file()
    {
        std::error_code ignored;
        fs::remove(path, ignored);
    }
};

struct command_line
{
    enum class action
    {
        compile, check, emit_llvm, emit_library_llvm, emit_analysis, test, profile
    } mode = action::compile;
    fs::path source_path;
    fs::path output_path;
    std::optional<fs::path> test_case;
    tx::test_options test_options;
    tx::profile_options profile_options;
    bool lto = true;
    bool windows_subsystem = false;

    command_line(action selected_mode, fs::path selected_source,
                 fs::path selected_output)
        : mode(selected_mode), source_path(std::move(selected_source)),
          output_path(std::move(selected_output))
    {
    }
};

std::optional<command_line> parse_command_line(const std::vector<std::string>& argv)
{
    const auto argc = static_cast<int>(argv.size());
    if (argc >= 3 && std::string(argv[1]) == "profile")
    {
        command_line result{command_line::action::profile, fs::u8path(argv[2]), {}};
        result.profile_options.output = executable_path().parent_path().parent_path() /
            "tx_build" / fs::u8path(path_text(result.source_path.stem()) + ".profile.json");
        for (int index = 3; index < argc; ++index)
        {
            const std::string option = argv[index];
            if (index + 1 >= argc)
            {
                return std::nullopt;
            }
            const std::string value = argv[++index];
            if (option == "-o")
            {
                result.profile_options.output = fs::u8path(value);
                continue;
            }
            if (value.empty() || value.size() > 8 ||
                value.find_first_not_of("0123456789") != std::string::npos)
            {
                return std::nullopt;
            }
            const auto number = std::stoul(value);
            if (option == "--warmup" && number <= 100)
            {
                result.profile_options.warmup = number;
            }
            else if (option == "--samples" && number >= 1 && number <= 100)
            {
                result.profile_options.samples = number;
            }
            else if (option == "--interval-ms" && number >= 1 && number <= 1000)
            {
                result.profile_options.interval_ms = number;
            }
            else if (option == "--timeout-ms" && number >= 1 && number <= 86400000)
            {
                result.profile_options.timeout_ms = number;
            }
            else
            {
                return std::nullopt;
            }
        }
        return result;
    }
    if (argc >= 3 && std::string(argv[1]) == "test")
    {
        command_line result{command_line::action::test, fs::u8path(argv[2]), {}};
        for (int index = 3; index < argc; ++index)
        {
            if (std::string(argv[index]) == "--case" &&
                !result.test_case && index + 1 < argc)
            {
                result.test_case = fs::u8path(argv[++index]);
            }
            else if (std::string(argv[index]) == "--format" &&
                     !result.test_options.json_format && index + 1 < argc &&
                     std::string(argv[index + 1]) == "json")
            {
                result.test_options.json_format = true;
                ++index;
            }
            else if ((std::string(argv[index]) == "--jobs" ||
                      std::string(argv[index]) == "--timeout-ms") && index + 1 < argc)
            {
                const bool jobs = std::string(argv[index]) == "--jobs";
                const std::string value = argv[++index];
                if (value.empty() || value.size() > 8 ||
                    value.find_first_not_of("0123456789") != std::string::npos)
                {
                    return std::nullopt;
                }
                const auto number = std::stoul(value);
                if (!number || number > (jobs ? 64u : 86400000u))
                {
                    return std::nullopt;
                }
                (jobs ? result.test_options.jobs : result.test_options.timeout_ms) = number;
            }
            else if (std::string(argv[index]) == "--isolation" && index + 1 < argc)
            {
                const std::string value = argv[++index];
                if (value != "workspace" && value != "source")
                {
                    return std::nullopt;
                }
                result.test_options.source_directory = value == "source";
            }
            else
            {
                return std::nullopt;
            }
        }
        return result;
    }
    if (argc == 3 && std::string(argv[1]) == "check")
    {
        return command_line{command_line::action::check, fs::u8path(argv[2]), {}};
    }
    if (argc >= 3 && (std::string(argv[1]) == "emit-llvm" ||
                      std::string(argv[1]) == "emit-library-llvm" ||
                      std::string(argv[1]) == "emit-analysis"))
    {
        if (argc != 3 && (argc != 5 || std::string(argv[3]) != "-o"))
        {
            return std::nullopt;
        }
        const fs::path source_path = fs::u8path(argv[2]);
        const fs::path output_path = argc == 5
            ? fs::u8path(argv[4])
            : executable_path().parent_path().parent_path() /
              "tx_build" / fs::u8path(path_text(source_path.stem()) +
                (std::string(argv[1]) == "emit-analysis" ? ".analysis.json" : ".ll"));
        return command_line{std::string(argv[1]) == "emit-library-llvm"
                ? command_line::action::emit_library_llvm
                : std::string(argv[1]) == "emit-analysis" ? command_line::action::emit_analysis
                : command_line::action::emit_llvm,
                            source_path, output_path};
    }
    if (argc < 2)
    {
        return std::nullopt;
    }
    const fs::path source_path = fs::u8path(argv[1]);
    command_line result{command_line::action::compile, source_path,
        executable_path().parent_path().parent_path() / "tx_build" /
        fs::u8path(tx::executable_name(path_text(source_path.stem())))};
    bool has_output = false;
    bool has_subsystem = false;
    for (int index = 2; index < argc; ++index)
    {
        const std::string option = argv[index];
        if (option == "-o" && !has_output && index + 1 < argc)
        {
            result.output_path = fs::u8path(argv[++index]);
            has_output = true;
        }
        else if (option == "--subsystem" && !has_subsystem && index + 1 < argc)
        {
            const auto& value = argv[++index];
            if (value != "console" && value != "windows")
            {
                throw std::runtime_error("--subsystem 只接受 console 或 windows");
            }
            result.windows_subsystem = value == "windows";
            has_subsystem = true;
        }
        else if (option == "--no-lto" && result.lto)
        {
            result.lto = false;
        }
        else
        {
            return std::nullopt;
        }
    }
    return result;
}

tx::program parse_and_check(const fs::path& source_path,
                            bool library_mode = false)
{
    tx::module_loader loader(executable_path().parent_path() / "stdlib");
    auto syntax = loader.load(source_path);
    tx::module_resolver resolver;
    resolver.resolve(syntax);
    tx::semantic_analyzer analyzer;
    analyzer.analyze(syntax, !library_mode && source_path.extension() != ".txh");
    return syntax;
}

int run_check(const fs::path& source_path)
{
    (void)parse_and_check(source_path);
    std::cout << "语法和类型检查通过：" << path_text(source_path) << '\n';
    return 0;
}


int run_emit_llvm(const fs::path& source_path, const fs::path& output_path,
                  bool library_mode = false, bool analysis_mode = false)
{
    if (source_path.extension() == ".txh")
    {
        throw std::runtime_error(".txh 是接口文件，不能单独生成可执行程序");
    }
    if (fs::exists(output_path) && fs::equivalent(source_path, output_path))
    {
        throw std::runtime_error("输出路径不能覆盖源码文件");
    }
    auto syntax = parse_and_check(source_path, library_mode);
    std::string generated_source;
    if (analysis_mode)
    {
        tx::program_analysis analysis;
        analysis.analyze(syntax);
        generated_source = analysis.dump();
    }
    else
    {
        tx::llvm_code_generator generator;
        generated_source = generator.generate(syntax, library_mode);
    }
    if (!output_path.parent_path().empty())
    {
        fs::create_directories(output_path.parent_path());
    }
    std::ofstream output(output_path, std::ios::binary);
    if (!output || !output.write(generated_source.data(),
                                 static_cast<std::streamsize>(generated_source.size())))
    {
        throw std::runtime_error("无法写入 LLVM IR 文件");
    }
    std::cout << "已生成 " << path_text(output_path) << '\n';
    return 0;
}

int run_compiler(const fs::path& source_path, const fs::path& output_path,
                 bool quiet = false, int profile_interval_ms = 0, bool lto = true,
                 bool windows_subsystem = false)
{
    if (source_path.extension() == ".txh")
    {
        throw std::runtime_error(".txh 是接口文件，不能单独编译为可执行文件");
    }
    if (fs::exists(output_path) && fs::equivalent(source_path, output_path))
    {
        throw std::runtime_error("输出路径不能覆盖源码文件");
    }
    auto syntax = parse_and_check(source_path);
    tx::llvm_code_generator generator;
    const auto generated_source = generator.generate(syntax, false, profile_interval_ms);
    if (!output_path.parent_path().empty())
    {
        fs::create_directories(output_path.parent_path());
    }
    const auto result = compile_llvm_native(generated_source, output_path, lto, windows_subsystem);
    if (result != 0)
    {
        std::cerr << "LLVM 后端编译失败（退出码 " << result << "）\n";
        return 1;
    }
    if (!quiet)
    {
        std::cout << "已生成 " << path_text(output_path) << '\n';
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[])
{
    std::optional<command_line> command;
    try
    {
        command = parse_command_line(tx::command_arguments(argc, argv));
        if (!command)
        {
            std::cerr << "用法：txc <源码.tx> [-o <输出程序>] [--no-lto] [--subsystem console|windows]\n"
                      << "      txc check <源码.tx>\n"
                      << "      txc emit-llvm <源码.tx> [-o <输出.ll>]\n"
                      << "      txc emit-library-llvm <源码.tx> -o <输出.ll>\n"
                      << "      txc emit-analysis <源码.tx> [-o <分析.json>]\n"
                      << "      txc test <目录或源码.tx> [--case <相对路径>] [--format json]\n"
                      << "          [--jobs 1..64] [--timeout-ms 1..86400000] [--isolation workspace|source]\n"
                      << "      txc profile <源码.tx> [-o 报告.json] [--warmup 次数] [--samples 次数]\n"
                      << "          [--interval-ms 毫秒] [--timeout-ms 毫秒]\n";
            return 2;
        }
        const auto tool_dir = executable_path().parent_path();
        if (command->mode == command_line::action::emit_library_llvm)
        {
            tx::verify_tool_interfaces(tool_dir);
        }
        else
        {
            tx::verify_tool_package(tool_dir);
        }
        switch (command->mode)
        {
        case command_line::action::check:
            return run_check(command->source_path);
        case command_line::action::emit_llvm:
            return run_emit_llvm(command->source_path, command->output_path);
        case command_line::action::emit_library_llvm:
            return run_emit_llvm(command->source_path, command->output_path, true);
        case command_line::action::emit_analysis:
            return run_emit_llvm(command->source_path, command->output_path, false, true);
        case command_line::action::compile:
            return run_compiler(command->source_path, command->output_path, false, 0,
                command->lto, command->windows_subsystem);
        case command_line::action::test:
            return tx::run_test_suite(command->source_path,
                command->test_case, command->test_options,
                [](const fs::path& source, const fs::path& output)
                {
                    return run_compiler(source, output, true);
                });
        case command_line::action::profile:
            return tx::run_profile(command->source_path, command->profile_options,
                [](const fs::path& source, const fs::path& output, int interval)
                {
                    return run_compiler(source, output, true, interval);
                });
        }
    }
    catch (const tx::compile_error& error)
    {
        const auto position = error.position();
        const auto file = position.file.empty()
            ? (command ? path_text(command->source_path) : "<unknown>")
            : position.file;
        std::cerr << file << ':' << position.line << ':'
                  << position.column << ": 错误：" << error.what() << '\n';
    }
    catch (const std::exception& error)
    {
        std::cerr << "错误：" << error.what() << '\n';
    }
    return 1;
}
