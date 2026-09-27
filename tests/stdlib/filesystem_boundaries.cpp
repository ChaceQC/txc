#include "stdlib/filesystem_extended.hpp"
#include "stdlib/filesystem_watch.hpp"
#include "stdlib/file_stream.hpp"
#include "stdlib/file_extra.hpp"
#include "stdlib/process_internal.hpp"
#include "stdlib/encoding.hpp"
#include "stdlib/error.hpp"

#include <winioctl.h>

#include <atomic>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <thread>

namespace
{

using namespace tx_generated;

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

std::string text_path(const std::filesystem::path& path)
{
    return detail::wide_to_utf8(path.wstring());
}

void create_file(const std::filesystem::path& path)
{
    process_detail::native_handle file(CreateFileW(path.c_str(), GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, CREATE_ALWAYS, 0, nullptr));
    require(file.valid(), "fixture create failed");
}

void permissions_and_short_read(const std::filesystem::path& root)
{
    const auto path = text_path(root / L"只读 短文件.bin");
    auto writer = open_binary_stream(path, "write");
    stream_write_bytes(writer, make_bytes({0, 1, 255}));
    stream_close(writer);
    auto reader = open_binary_stream(path, "read");
    const auto first = stream_read_bytes(reader, 65536);
    require(first.data->size() == 3 && !first.eof, "short file read failed");
    require(stream_read_bytes(reader, 65536).eof, "file EOF failed");
    stream_close(reader);
    fs_set_permissions(path, 0444);
    bool denied = false;
    try
    {
        open_binary_stream(path, "write");
    }
    catch (const runtime_failure& error)
    {
        denied = error.error().code == "permission_denied";
    }
    fs_set_permissions(path, 0666);
    require(denied, "readonly permission did not reject write");
    std::cout << "FILESYSTEM_PERMISSION_SHORT_READ_OK\n";
}

void watch_overflow(const std::filesystem::path& root)
{
    const auto directory = root / L"监视 溢出";
    std::filesystem::create_directory(directory);
    const auto watcher = fs_watch(text_path(directory), false);
    for (int index = 0; index < 1600; ++index)
    {
        create_file(directory / (std::wstring(120, L'x') + std::to_wstring(index)));
    }
    bool overflow = false;
    for (int index = 0; index < 4000; ++index)
    {
        const auto event = fs_watch_next(watcher, 100);
        overflow = overflow || event.kind == "overflow";
        if (event.kind == "timeout")
        {
            break;
        }
    }
    require(overflow, "kernel watch overflow was not reported");
    create_file(directory / L"after-overflow");
    require(fs_watch_next(watcher, 2000).kind == "created", "watch did not recover");
    fs_close_watch(watcher);
    fs_close_watch(watcher);
    std::cout << "FILESYSTEM_WATCH_OVERFLOW_OK\n";
}

void watch_close_wakes(const std::filesystem::path& root)
{
    auto watcher = fs_watch(text_path(root), false);
    std::atomic<bool> entered = false;
    bool closed = false;
    std::jthread waiter([&]
    {
        entered = true;
        try
        {
            fs_watch_next(watcher, -1);
        }
        catch (const runtime_failure& error)
        {
            closed = error.error().code == "closed_handle";
        }
    });
    while (!entered)
    {
        std::this_thread::yield();
    }
    Sleep(10);
    fs_close_watch(watcher);
    waiter.join();
    require(closed, "closing watch did not wake pending read");
    std::cout << "FILESYSTEM_WATCH_CLOSE_OK\n";
}

void symbolic_link_boundary(const std::filesystem::path& root)
{
    const auto target = root / L"符号链接 目标.bin";
    const auto link = root / L"符号链接 本身.bin";
    create_file(target);
    try
    {
        fs_create_symlink(text_path(target), text_path(link), false);
    }
    catch (const runtime_failure& error)
    {
        require(error.error().code == "permission_denied", "unexpected symlink error");
        std::cout << "FILESYSTEM_SYMLINK_PERMISSION_DENIED\n";
        return;
    }
    const bool observed = fs_stat(text_path(link), false).kind == "symlink" &&
        fs_stat(text_path(link), true).kind == "file";
    DeleteFileW(link.c_str());
    require(observed, "symbolic link stat mismatch");
    std::cout << "FILESYSTEM_SYMLINK_OK\n";
}

void set_junction(const std::filesystem::path& link, const std::filesystem::path& target)
{
    struct junction_data
    {
        DWORD tag = IO_REPARSE_TAG_MOUNT_POINT;
        WORD size = 0;
        WORD reserved = 0;
        WORD substitute_offset = 0;
        WORD substitute_length = 0;
        WORD print_offset = 0;
        WORD print_length = 0;
        wchar_t names[4096]{};
    } data;
    const auto printable = target.wstring();
    const auto substitute = L"\\??\\" + printable;
    require(substitute.size() + printable.size() + 2 < 4096, "junction path too long");
    data.substitute_length = static_cast<WORD>(substitute.size() * sizeof(wchar_t));
    data.print_offset = data.substitute_length + sizeof(wchar_t);
    data.print_length = static_cast<WORD>(printable.size() * sizeof(wchar_t));
    data.size = 8 + data.print_offset + data.print_length + sizeof(wchar_t);
    std::memcpy(data.names, substitute.data(), data.substitute_length);
    std::memcpy(reinterpret_cast<char*>(data.names) + data.print_offset,
        printable.data(), data.print_length);
    process_detail::native_handle handle(CreateFileW(link.c_str(), GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr));
    DWORD received = 0;
    require(handle.valid() && DeviceIoControl(handle.get(), FSCTL_SET_REPARSE_POINT,
        &data, data.size + 8, nullptr, 0, &received, nullptr), "junction update failed");
}

void junction_race(const std::filesystem::path& root)
{
    const auto first = root / L"目标 一";
    const auto second = root / L"目标 二";
    const auto link = root / L"联接 竞争";
    std::filesystem::create_directory(first);
    std::filesystem::create_directory(second);
    std::filesystem::create_directory(link);
    struct link_cleanup
    {
        std::filesystem::path path;
        ~link_cleanup()
        {
            // 只删除测试联接本身，绝不递归进入目标。
            RemoveDirectoryW(path.c_str());
        }
    } cleanup{link};
    auto one = open_binary_stream(text_path(first / L"value"), "write");
    stream_write_bytes(one, make_bytes({1}));
    stream_close(one);
    auto two = open_binary_stream(text_path(second / L"value"), "write");
    stream_write_bytes(two, make_bytes({1, 2, 3}));
    stream_close(two);
    set_junction(link, first);
    std::atomic<bool> failed = false;
    std::jthread changing([&](std::stop_token stop)
    {
        try
        {
            while (!stop.stop_requested())
            {
                set_junction(link, second);
                set_junction(link, first);
            }
        }
        catch (...)
        {
            failed = true;
        }
    });
    for (int index = 0; index < 200; ++index)
    {
        require(fs_stat(text_path(link), false).kind == "symlink", "lstat followed link");
        require(fs_stat(text_path(link), true).kind == "directory", "stat did not follow link");
        const auto size = fs_stat(text_path(link / L"value"), true).size;
        require(size == 1 || size == 3, "racing metadata was invalid");
        const auto target = detail::path_from_utf8(fs_read_symlink(text_path(link)));
        require(target == first || target == second, "racing link target invalid");
    }
    changing.request_stop();
    changing.join();
    require(!failed, "junction mutation failed");
    std::filesystem::rename(first, root / L"移走目标");
    set_junction(link, first);
    require(fs_stat(text_path(link), false).kind == "symlink", "dangling link lost identity");
    bool missing = false;
    try
    {
        fs_stat(text_path(link), true);
    }
    catch (const runtime_failure& error)
    {
        missing = error.error().code == "not_found";
    }
    require(missing, "dangling link did not report not_found");
    std::cout << "FILESYSTEM_LINK_RACE_OK\n";
}

} // namespace

int main()
{
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    try
    {
        const auto root = std::filesystem::current_path();
        permissions_and_short_read(root);
        junction_race(root);
        symbolic_link_boundary(root);
        watch_overflow(root);
        watch_close_wakes(root);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
