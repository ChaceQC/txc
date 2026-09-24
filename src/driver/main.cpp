#include "backend/cpp/codegen.hpp"
#include "driver/module_loader.hpp"
#include "frontend/resolver/module_resolver.hpp"
#include "frontend/sema/sema.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <process.h>
#include <stdexcept>
#include <string>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace
{

namespace fs = std::filesystem;

fs::path executable_path();

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
    bool check_only = false;
    fs::path source_path;
    fs::path output_path;
};

std::optional<command_line> parse_command_line(int argc, char* argv[])
{
    if (argc == 3 && std::string(argv[1]) == "check")
    {
        return command_line{true, argv[2], {}};
    }
    if (argc != 2 && (argc != 4 || std::string(argv[2]) != "-o"))
    {
        return std::nullopt;
    }
    const fs::path source_path = argv[1];
    const fs::path output_path = argc == 4
        ? fs::path(argv[3])
        : executable_path().parent_path().parent_path() /
          "tx_build" / (source_path.stem().string() + ".exe");
    return command_line{false, source_path, output_path};
}

fs::path executable_path()
{
    std::wstring buffer(MAX_PATH, L'\0');
    while (true)
    {
        const auto length = GetModuleFileNameW(
            nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0)
        {
            throw std::runtime_error("无法定位 txc 可执行文件");
        }
        if (length < buffer.size())
        {
            buffer.resize(length);
            return fs::path(buffer);
        }
        buffer.resize(buffer.size() * 2);
    }
}

fs::path temporary_cpp_path()
{
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto prefix = "txc_" + std::to_string(_getpid()) + "_" + std::to_string(stamp);
    for (int suffix = 0; suffix < 100; ++suffix)
    {
        const auto candidate = fs::temp_directory_path() /
            (prefix + "_" + std::to_string(suffix) + ".cpp");
        if (!fs::exists(candidate))
        {
            return candidate;
        }
    }
    throw std::runtime_error("无法创建临时 C++ 文件名");
}

int compile_native(const std::string& generated_source, const fs::path& output_path)
{
    const auto library = executable_path().parent_path() / "libtxstdlib.a";
    if (!fs::exists(library))
    {
        throw std::runtime_error("缺少标准库二进制：" + library.string());
    }
    temporary_file generated{temporary_cpp_path()};
    {
        std::ofstream output(generated.path, std::ios::binary);
        if (!output)
        {
            throw std::runtime_error("无法创建临时 C++ 文件");
        }
        output.write(generated_source.data(),
                     static_cast<std::streamsize>(generated_source.size()));
        if (!output)
        {
            throw std::runtime_error("无法写入临时 C++ 文件");
        }
    }

    const std::vector<std::string> arguments = {
        "g++", "-std=c++23", "-O2", "-finput-charset=UTF-8",
        "-fexec-charset=UTF-8", generated.path.string(), library.string(),
        "-o", output_path.string()};
    std::vector<const char*> raw_arguments;
    raw_arguments.reserve(arguments.size() + 1);
    for (const auto& argument : arguments)
    {
        raw_arguments.push_back(argument.c_str());
    }
    raw_arguments.push_back(nullptr);
    const auto result = _spawnvp(_P_WAIT, "g++", raw_arguments.data());
    if (result == -1)
    {
        throw std::runtime_error("无法启动 g++，请确认它已加入 PATH");
    }
    return result;
}

tx::program parse_and_check(const fs::path& source_path)
{
    tx::module_loader loader(executable_path().parent_path() / "stdlib");
    auto syntax = loader.load(source_path);
    tx::module_resolver resolver;
    resolver.resolve(syntax);
    tx::semantic_analyzer analyzer;
    analyzer.analyze(syntax, source_path.extension() != ".txh");
    return syntax;
}

int run_check(const fs::path& source_path)
{
    (void)parse_and_check(source_path);
    std::cout << "语法和类型检查通过：" << source_path.string() << '\n';
    return 0;
}

int run_compiler(const fs::path& source_path, const fs::path& output_path)
{
    if (source_path.extension() == ".txh")
    {
        throw std::runtime_error(".txh 是接口文件，不能单独编译为可执行文件");
    }
    if (fs::exists(output_path) && fs::equivalent(source_path, output_path))
    {
        throw std::runtime_error("输出路径不能覆盖源码文件");
    }
    if (!output_path.parent_path().empty())
    {
        fs::create_directories(output_path.parent_path());
    }
    auto syntax = parse_and_check(source_path);
    tx::code_generator generator;
    const auto generated_source = generator.generate(syntax);
    const auto result = compile_native(generated_source, output_path);
    if (result != 0)
    {
        std::cerr << "后端 C++ 编译失败（退出码 " << result << "）\n";
        return 1;
    }
    std::cout << "已生成 " << output_path.string() << '\n';
    return 0;
}

} // namespace

int main(int argc, char* argv[])
{
    const auto command = parse_command_line(argc, argv);
    if (!command)
    {
        std::cerr << "用法：txc <源码.tx> [-o <输出.exe>]\n"
                  << "      txc check <源码.tx>\n";
        return 2;
    }
    try
    {
        return command->check_only
            ? run_check(command->source_path)
            : run_compiler(command->source_path, command->output_path);
    }
    catch (const tx::compile_error& error)
    {
        const auto position = error.position();
        const auto file = position.file.empty()
            ? command->source_path.string() : position.file;
        std::cerr << file << ':' << position.line << ':'
                  << position.column << ": 错误：" << error.what() << '\n';
    }
    catch (const std::exception& error)
    {
        std::cerr << "错误：" << error.what() << '\n';
    }
    return 1;
}
