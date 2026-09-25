#include "backend/llvm/codegen.hpp"
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
#include <string_view>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

namespace
{

namespace fs = std::filesystem;

std::string path_text(const fs::path& path)
{
    const auto utf8 = path.u8string();
    return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}

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
    enum class action { compile, check, emit_llvm } mode = action::compile;
    fs::path source_path;
    fs::path output_path;
};

struct windows_arguments
{
    LPWSTR* values;

    ~windows_arguments()
    {
        if (values)
        {
            LocalFree(values);
        }
    }
};

std::optional<command_line> parse_command_line(int argc, wchar_t* argv[])
{
    if (argc == 3 && std::wstring(argv[1]) == L"check")
    {
        return command_line{command_line::action::check, argv[2], {}};
    }
    if (argc >= 3 && std::wstring(argv[1]) == L"emit-llvm")
    {
        if (argc != 3 && (argc != 5 || std::wstring(argv[3]) != L"-o"))
        {
            return std::nullopt;
        }
        const fs::path source_path = argv[2];
        const fs::path output_path = argc == 5
            ? fs::path(argv[4])
            : executable_path().parent_path().parent_path() /
              "tx_build" / (source_path.stem().wstring() + L".ll");
        return command_line{command_line::action::emit_llvm,
                            source_path, output_path};
    }
    if (argc != 2 && (argc != 4 || std::wstring(argv[2]) != L"-o"))
    {
        return std::nullopt;
    }
    const fs::path source_path = argv[1];
    const fs::path output_path = argc == 4
        ? fs::path(argv[3])
        : executable_path().parent_path().parent_path() /
          "tx_build" / (source_path.stem().wstring() + L".exe");
    return command_line{command_line::action::compile,
                        source_path, output_path};
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

fs::path temporary_path(std::string_view extension)
{
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto prefix = "txc_" + std::to_string(_getpid()) + "_" + std::to_string(stamp);
    for (int suffix = 0; suffix < 100; ++suffix)
    {
        const auto candidate = fs::temp_directory_path() /
            (prefix + "_" + std::to_string(suffix) + std::string(extension));
        if (!fs::exists(candidate))
        {
            return candidate;
        }
    }
    throw std::runtime_error("无法创建临时文件名");
}


int run_local_tool(const fs::path& executable,
                   const std::vector<std::wstring>& parameters)
{
    if (!fs::exists(executable))
    {
        throw std::runtime_error("缺少编译组件：" + path_text(executable));
    }
    std::vector<std::wstring> arguments;
    arguments.reserve(parameters.size() + 1);
    arguments.push_back(executable.wstring());
    arguments.insert(arguments.end(), parameters.begin(), parameters.end());
    std::vector<const wchar_t*> raw_arguments;
    raw_arguments.reserve(arguments.size() + 1);
    for (const auto& argument : arguments)
    {
        raw_arguments.push_back(argument.c_str());
    }
    raw_arguments.push_back(nullptr);
    const auto result = _wspawnv(_P_WAIT, executable.wstring().c_str(),
                                 raw_arguments.data());
    if (result == -1)
    {
        throw std::runtime_error("无法启动编译组件：" + path_text(executable));
    }
    return result;
}

void place_runtime_dependency(const fs::path& source,
                              const fs::path& output_dir)
{
    const auto destination = output_dir / source.filename();
    if (fs::exists(destination))
    {
        return;
    }
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    temporary_file staged{output_dir /
        (".txc_" + source.filename().string() + "_" +
         std::to_string(_getpid()) + "_" + std::to_string(stamp) + ".tmp")};
    fs::copy_file(source, staged.path);
    if (!MoveFileW(staged.path.c_str(), destination.c_str()))
    {
        const auto error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS && error != ERROR_FILE_EXISTS)
        {
            throw std::runtime_error("无法放置运行时依赖：" + path_text(destination));
        }
    }
}

int compile_llvm_native(const std::string& generated_source,
                        const fs::path& output_path)
{
    const auto tool_dir = executable_path().parent_path();
    const auto link_dir = tool_dir / "link";
    const auto library = tool_dir / "libtxstdlib.a";
    if (!fs::exists(library))
    {
        throw std::runtime_error("缺少标准库二进制：" + path_text(library));
    }
    temporary_file ir_file{temporary_path(".ll")};
    temporary_file object_file{temporary_path(".o")};
    {
        std::ofstream output(ir_file.path, std::ios::binary);
        if (!output || !output.write(generated_source.data(),
                                     static_cast<std::streamsize>(generated_source.size())))
        {
            throw std::runtime_error("无法写入临时 LLVM IR 文件");
        }
    }
    const auto emit_result = run_local_tool(tool_dir / "clang.exe",
        {L"-target", L"x86_64-w64-windows-gnu", L"-x", L"ir", L"-c",
         L"-O3", L"-o", object_file.path.wstring(), ir_file.path.wstring()});
    if (emit_result != 0)
    {
        return emit_result;
    }

    std::vector<std::wstring> arguments = {
        L"-m", L"i386pep", L"-Bdynamic", L"-o", output_path.wstring(),
        (link_dir / "crt2.o").wstring(),
        (link_dir / "crtbegin.o").wstring(),
        L"-L" + link_dir.wstring(),
        object_file.path.wstring(), library.wstring(),
        L"-lstdc++", L"-lmingw32", L"-lgcc_s", L"-lgcc",
        L"-lmoldname", L"-lmingwex", L"-lmsvcrt", L"-lkernel32",
        L"-lpthread", L"-ladvapi32", L"-lshell32", L"-luser32",
        L"-lkernel32", L"-liconv", L"-lmingw32", L"-lgcc_s",
        L"-lgcc", L"-lmoldname", L"-lmingwex", L"-lmsvcrt",
        L"-lkernel32", (link_dir / "default-manifest.o").wstring(),
        (link_dir / "crtend.o").wstring()
    };
    const auto link_result = run_local_tool(link_dir / "ld.exe", arguments);
    if (link_result != 0)
    {
        return link_result;
    }
    const auto output_dir = output_path.parent_path().empty()
        ? fs::current_path() : output_path.parent_path();
    for (const auto* name : {"libgcc_s_seh-1.dll", "libstdc++-6.dll",
                             "libwinpthread-1.dll"})
    {
        place_runtime_dependency(tool_dir / name, output_dir);
    }
    return 0;
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
    std::cout << "语法和类型检查通过：" << path_text(source_path) << '\n';
    return 0;
}


int run_emit_llvm(const fs::path& source_path, const fs::path& output_path)
{
    if (source_path.extension() == ".txh")
    {
        throw std::runtime_error(".txh 是接口文件，不能单独生成可执行程序");
    }
    if (fs::exists(output_path) && fs::equivalent(source_path, output_path))
    {
        throw std::runtime_error("输出路径不能覆盖源码文件");
    }
    auto syntax = parse_and_check(source_path);
    tx::llvm_code_generator generator;
    const auto generated_source = generator.generate(syntax);
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
    auto syntax = parse_and_check(source_path);
    tx::llvm_code_generator generator;
    const auto generated_source = generator.generate(syntax);
    if (!output_path.parent_path().empty())
    {
        fs::create_directories(output_path.parent_path());
    }
    const auto result = compile_llvm_native(generated_source, output_path);
    if (result != 0)
    {
        std::cerr << "LLVM 后端编译失败（退出码 " << result << "）\n";
        return 1;
    }
    std::cout << "已生成 " << path_text(output_path) << '\n';
    return 0;
}

} // namespace

int main()
{
    std::optional<command_line> command;
    try
    {
        int argc = 0;
        windows_arguments arguments{CommandLineToArgvW(GetCommandLineW(), &argc)};
        if (!arguments.values)
        {
            throw std::runtime_error("无法读取 Windows 命令行参数");
        }
        command = parse_command_line(argc, arguments.values);
        if (!command)
        {
            std::cerr << "用法：txc <源码.tx> [-o <输出.exe>]\n"
                      << "      txc check <源码.tx>\n"
                      << "      txc emit-llvm <源码.tx> [-o <输出.ll>]\n";
            return 2;
        }
        switch (command->mode)
        {
        case command_line::action::check:
            return run_check(command->source_path);
        case command_line::action::emit_llvm:
            return run_emit_llvm(command->source_path, command->output_path);
        case command_line::action::compile:
            return run_compiler(command->source_path, command->output_path);
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
