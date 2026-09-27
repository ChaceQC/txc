#include "stdlib/encoding.hpp"
#include "stdlib/file_stream.hpp"
#include "stdlib/regex.hpp"
#include "stdlib/error.hpp"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

namespace
{

void require(bool condition)
{
    if (!condition)
    {
        throw std::runtime_error("运行时工作区边界验证失败");
    }
}

void encoding_boundaries()
{
    using namespace tx_generated::detail;
    for (const auto text : {"\xc0\x80", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2\x82", "\x80"})
    {
        bool failed = false;
        try
        {
            validate_utf8(text);
        }
        catch (const std::runtime_error&)
        {
            failed = true;
        }
        require(failed);
    }
    const std::string text("a\0\xf0\x9f\x8c\x8d", 6);
    require(decode_text(encode_text(text, text_encoding::utf8), text_encoding::utf8) == text);
}

void stream_boundaries()
{
    using namespace tx_generated;
    const std::filesystem::path path = "tx_build/static_runtime_checks/native_stream.bin";
    const std::string expected(200000, 'x');
    {
        std::ofstream file(path, std::ios::binary);
        file << expected;
    }
    {
        stream_file file(path.string(), stream_mode::read, false);
        require(file.read(8 * 1024 * 1024) == expected);
        require(file.read(8 * 1024 * 1024).empty());
        require(file.read(8 * 1024 * 1024).empty());
    }
    {
        stream_file file(path.string(), stream_mode::write, false);
        bool failed = false;
        try
        {
            (void)file.read(0);
        }
        catch (const runtime_failure& error)
        {
            failed = error.error().code == "invalid_mode";
        }
        require(failed);
    }
    std::filesystem::remove(path);
}

void regex_workspaces()
{
    using namespace tx_generated;
    const auto small = regex_compile("(a)?b", "", 1024, 10000, 1000);
    const auto large = regex_compile("(a)(b)(c)(d)(e)", "", 1024, 10000, 1000);
    std::atomic<bool> valid = true;
    const auto work = [&]
    {
        try
        {
            for (int repeat = 0; repeat < 100; ++repeat)
            {
                require(regex_search(large, "abcde", 0).groups.size() == 6);
                const auto match = regex_search(small, "b", 0);
                require(match.found && match.groups.size() == 2 &&
                    match.group_start_bytes[1] == -1 && match.groups[1].empty());
                require(!regex_search(small, "x", 0).found);
            }
        }
        catch (...)
        {
            valid = false;
        }
    };
    std::jthread first(work);
    std::jthread second(work);
    work();
    first.join();
    second.join();
    require(valid.load());
    auto state = std::make_shared<cancellation_state>();
    const auto cancelled = regex_compile("a", "", 1024, 10000, 1000, state);
    state->cancelled = true;
    bool failed = false;
    try
    {
        (void)regex_search(cancelled, "a", 0);
    }
    catch (const runtime_failure& error)
    {
        failed = error.error().kind == tx::error_kind::cancelled;
    }
    require(failed && regex_search(small, "ab", 0).found);
}

} // namespace

int main()
{
    encoding_boundaries();
    stream_boundaries();
    regex_workspaces();
    std::cout << "NATIVE_WORKSPACE_OK\n";
}
