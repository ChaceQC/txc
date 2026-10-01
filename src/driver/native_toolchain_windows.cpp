#include "driver/native_toolchain.hpp"
#include <chrono>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <process.h>
#include <stdexcept>
#include <string_view>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
namespace tx
{
namespace fs = std::filesystem;
std::string path_text(const fs::path& path)
{
    const auto utf8 = path.u8string();
    return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}


struct temporary_file
{
    fs::path path;

    ~temporary_file()
    {
        std::error_code ignored;
        fs::remove(path, ignored);
    }
};

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


std::wstring quote_tool_argument(std::wstring_view argument)
{
    std::wstring result = L"\"";
    std::size_t backslashes = 0;
    for (const auto character : argument)
    {
        if (character == L'\\')
        {
            ++backslashes;
            continue;
        }
        // Windows CRT 在引号前将成对反斜杠还原，奇数个反斜杠转义引号。
        result.append(character == L'\"' ? backslashes * 2 + 1 : backslashes, L'\\');
        result.push_back(character);
        backslashes = 0;
    }
    // 尾部反斜杠需要成对传递，避免把我们添加的结束引号转义掉。
    result.append(backslashes * 2, L'\\');
    result.push_back(L'\"');
    return result;
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
    // _wspawnv 只用空格拼接参数，调用方必须为每一项保留命令行边界。
    arguments.push_back(quote_tool_argument(executable.wstring()));
    for (const auto& parameter : parameters)
    {
        arguments.push_back(quote_tool_argument(parameter));
    }
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

bool same_runtime_dependency(const fs::path& source,
                             const fs::path& destination)
{
    if (!fs::exists(destination) || fs::file_size(source) != fs::file_size(destination))
    {
        return false;
    }
    if (fs::equivalent(source, destination))
    {
        return true;
    }
    std::ifstream left(source, std::ios::binary);
    std::ifstream right(destination, std::ios::binary);
    if (!left || !right)
    {
        throw std::runtime_error("无法核对运行时依赖文件");
    }
    std::array<char, 65536> first{};
    std::array<char, 65536> second{};
    while (left)
    {
        left.read(first.data(), first.size());
        const auto count = left.gcount();
        right.read(second.data(), count);
        if (right.gcount() != count || std::memcmp(first.data(), second.data(),
                                                   static_cast<std::size_t>(count)) != 0)
        {
            return false;
        }
    }
    return true;
}

void place_runtime_dependency(const fs::path& source,
                              const fs::path& output_dir)
{
    const auto destination = output_dir / source.filename();
    if (same_runtime_dependency(source, destination))
    {
        return;
    }
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    temporary_file staged{output_dir /
        (".txc_" + source.filename().string() + "_" +
         std::to_string(_getpid()) + "_" + std::to_string(stamp) + ".tmp")};
    fs::copy_file(source, staged.path);
    if (!MoveFileExW(staged.path.c_str(), destination.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        throw std::runtime_error("无法更新运行时依赖：" + path_text(destination));
    }
}

int compile_llvm_native(const std::string& generated_source,
                        const fs::path& output_path, bool lto)
{
    const auto tool_dir = executable_path().parent_path();
    const auto link_dir = tool_dir / "link";
    const auto library = tool_dir / (lto ? "libtxstdlib_lto.a" : "libtxstdlib.a");
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
    std::vector<std::wstring> compile_arguments{
        L"-target", L"x86_64-w64-windows-gnu", L"-x", L"ir", L"-c",
        L"-O3", L"-o", object_file.path.wstring(), ir_file.path.wstring()};
    if (lto)
    {
        compile_arguments.push_back(L"-flto=thin");
    }
    const auto emit_result = run_local_tool(tool_dir / "clang.exe", compile_arguments);
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
        L"-lpthread", L"-ladvapi32", L"-lbcrypt", L"-lcrypt32", L"-lncrypt",
        L"-lwinhttp", L"-lws2_32", L"-ldnsapi",
        L"-lshell32", L"-luser32",
        L"-lkernel32", L"-liconv", L"-lmingw32", L"-lgcc_s",
        L"-lgcc", L"-lmoldname", L"-lmingwex", L"-lmsvcrt",
        L"-lkernel32", (link_dir / "default-manifest.o").wstring(),
        (link_dir / "crtend.o").wstring()
    };
    if (lto)
    {
        arguments.push_back(L"--lto-O3");
        arguments.push_back(L"--thinlto-jobs=8");
        // ThinLTO 在链接阶段生成机器码，此处也必须与 MinGW emutls ABI 一致。
        arguments.push_back(L"--plugin-opt=-emulated-tls");
    }
    // 两种模式都由 LLD 处理宽字符路径，避免 GNU ld 按系统代码页丢失中文。
    const auto link_result = run_local_tool(link_dir / "ld.lld.exe", arguments);
    if (link_result != 0)
    {
        return link_result;
    }
    const auto output_dir = output_path.parent_path().empty()
        ? fs::current_path() : output_path.parent_path();
    for (const auto* name : {"libgcc_s_seh-1.dll", "libstdc++-6.dll",
                             "libwinpthread-1.dll", "libstdc++-u.dll",
                             "libwinpthread-u.dll",
                             "libicuin78.dll",
                             "libicuuc78.dll", "libicudt78.dll",
                             "msquic.dll", "libpq.dll", "libssl-3-x64.dll",
                             "libcrypto-3-x64.dll", "libintl-9.dll", "libiconv-2.dll",
                             "libwinpthread-p.dll", "vcruntime140.dll"})
    {
        place_runtime_dependency(tool_dir / name, output_dir);
    }
    return 0;
}


std::vector<std::string> command_arguments(int, char*[])
{
    int count = 0;
    auto* raw = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!raw)
    {
        throw std::runtime_error("无法读取 Windows 命令行参数");
    }
    struct argument_owner
    {
        LPWSTR* values;
        ~argument_owner()
        {
            LocalFree(values);
        }
    } owner{raw};
    std::vector<std::string> result;
    for (int index = 0; index < count; ++index)
    {
        const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
            raw[index], -1, nullptr, 0, nullptr, nullptr);
        if (!length)
        {
            throw std::runtime_error("无法转换命令行参数");
        }
        std::string value(length, '\0');
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, raw[index], -1,
            value.data(), length, nullptr, nullptr);
        value.pop_back();
        result.push_back(std::move(value));
    }
    return result;
}

} // namespace tx
