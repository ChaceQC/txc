#include "driver/native_toolchain.hpp"
#include "common/platform.hpp"

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <sys/wait.h>
#include <unistd.h>

namespace tx
{
namespace
{

namespace fs = std::filesystem;

struct temporary_file
{
    fs::path path;
    ~temporary_file()
    {
        std::error_code ignored;
        fs::remove(path, ignored);
    }
};

int run_tool(const fs::path& executable, std::vector<std::string> arguments)
{
    arguments.insert(arguments.begin(), executable.string());
    std::vector<char*> values;
    for (auto& argument : arguments)
    {
        values.push_back(argument.data());
    }
    values.push_back(nullptr);
    const auto child = fork();
    if (child < 0)
    {
        throw std::system_error(errno, std::generic_category(), "无法创建编译进程");
    }
    if (child == 0)
    {
        execv(executable.c_str(), values.data());
        _exit(127);
    }
    int status = 0;
    while (waitpid(child, &status, 0) < 0)
    {
        if (errno != EINTR)
        {
            throw std::system_error(errno, std::generic_category(), "无法等待编译进程");
        }
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}

void install_runtime(const fs::path& tool_dir, const fs::path& output_dir)
{
    const auto source_dir = tool_dir / "lib";
    const auto destination = output_dir / "tx_lib";
    fs::create_directories(destination);
    for (const auto& entry : fs::directory_iterator(source_dir))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }
        const auto target = destination / entry.path().filename();
        if (fs::exists(target) && fs::equivalent(entry.path(), target))
        {
            continue;
        }
        // rename 原子替换，避免运行中的程序看到只复制了一部分的共享库。
        const auto staged = destination / ("." + entry.path().filename().string() +
            "." + std::to_string(process_id()) + ".tmp");
        temporary_file cleanup{staged};
        fs::copy_file(entry.path(), staged, fs::copy_options::overwrite_existing);
        fs::rename(staged, target);
    }
}

} // namespace

std::vector<std::string> command_arguments(int argc, char* argv[])
{
    return {argv, argv + argc};
}

std::filesystem::path executable_path()
{
    std::error_code error;
    const auto path = fs::read_symlink("/proc/self/exe", error);
    if (error)
    {
        throw std::system_error(error, "无法定位 txc 可执行文件");
    }
    return path;
}

std::filesystem::path temporary_path(std::string_view extension)
{
    std::string pattern = (fs::temp_directory_path() / "txc_XXXXXX").string() +
        std::string(extension);
    const int descriptor = mkstemps(pattern.data(), static_cast<int>(extension.size()));
    if (descriptor < 0)
    {
        throw std::system_error(errno, std::generic_category(), "无法创建编译临时文件");
    }
    close(descriptor);
    return pattern;
}

int compile_llvm_native(const std::string& generated_source,
    const std::filesystem::path& output_path, bool lto)
{
    const auto tool_dir = executable_path().parent_path();
    const auto link_dir = tool_dir / "link";
    temporary_file ir{temporary_path(".ll")};
    temporary_file object{temporary_path(".o")};
    {
        std::ofstream output(ir.path, std::ios::binary);
        if (!output.write(generated_source.data(), generated_source.size()))
        {
            throw std::runtime_error("无法写入临时 LLVM IR 文件");
        }
    }
    std::vector<std::string> compile{"-target", std::string(target_triple),
        "-x", "ir", "-c", "-O3", "-fPIC", "-o", object.path.string(), ir.path.string()};
    if (lto)
    {
        compile.push_back("-flto=thin");
    }
    if (const auto code = run_tool(tool_dir / "clang", std::move(compile)))
    {
        return code;
    }
    std::vector<std::string> link{"-m", "elf_x86_64", "--eh-frame-hdr", "--dynamic-linker",
        "/lib64/ld-linux-x86-64.so.2", "-o", output_path.string(),
        (link_dir / "crt1.o").string(), (link_dir / "crti.o").string(),
        (link_dir / "crtbegin.o").string(), "-L" + link_dir.string(),
        "-L" + (tool_dir / "lib").string(), "-L/lib/x86_64-linux-gnu",
        "-L/usr/lib/x86_64-linux-gnu", object.path.string(),
        (tool_dir / (lto ? "libtxstdlib_lto.a" : "libtxstdlib.a")).string(),
        "-rpath", "$ORIGIN/tx_lib", "--disable-new-dtags", "--start-group",
        "-l:libstdc++.so.6", "-l:libm.so.6", "-l:libgcc_s.so.1", "-lgcc", "-l:libc.so.6",
        "-licui18n", "-licuuc", "-licudata", "-lpcre2-8", "-lsodium",
        "-largon2", "-lxml2", "-lpq", "-lcurl",
        "-lssl", "-lcrypto", "-lcares", "-lz", "--end-group",
        (link_dir / "crtend.o").string(), (link_dir / "crtn.o").string()};
    if (lto)
    {
        link.insert(link.end(), {"--lto-O3", "--thinlto-jobs=8"});
    }
    const auto code = run_tool(link_dir / "ld.lld", std::move(link));
    if (!code)
    {
        install_runtime(tool_dir, fs::absolute(output_path).parent_path());
    }
    return code;
}

} // namespace tx
