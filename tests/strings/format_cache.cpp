#include "stdlib/format_internal.hpp"
#include "stdlib/format_plan_cache.hpp"
#include "stdlib/stdlib.hpp"
#include "backend/cpp/format_static_abi.hpp"
#include "backend/cpp/runtime_abi.hpp"
#include "backend/cpp/runtime_context.hpp"

#include <atomic>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

using namespace tx_generated;

namespace
{

void require(bool value)
{
    if (!value)
    {
        throw std::runtime_error("format cache check failed");
    }
}

std::string format(const std::string& text, std::initializer_list<std::any> values)
{
    return tx_fn_format(text, tx_array(values.begin(), values.end()), tx_dict{});
}

void expect_error(const std::string& text, std::initializer_list<std::any> values,
    const std::string& message)
{
    try
    {
        (void)format(text, values);
    }
    catch (const std::runtime_error& error)
    {
        require(error.what() == message);
        return;
    }
    throw std::runtime_error("missing format error");
}

void errors_and_values()
{
    require(format("{}:{}:{}", {std::numeric_limits<std::int64_t>::min(), false,
        std::string("甲\0乙", 7)}) == "-9223372036854775808:false:" + std::string("甲\0乙", 7));
    require(format("{:d}", {std::int64_t{3}}) == "3");
    expect_error("{:d}", {std::string("bad")}, "format 整数格式需要 int，且不支持精度");
    expect_error("{:d}", {}, "format 缺少位置参数");
    expect_error("{} {", {}, "format 缺少位置参数");
    expect_error("{:bad}", {}, "format 缺少位置参数");
    expect_error("{!z}", {}, "format 只支持 !s 和 !r 转换");
    expect_error("{} {", {std::int64_t{1}}, "format 替换字段缺少匹配的右大括号");
    require(!find_format_plan("{} {"));
    require(format("{}", {std::string("valid")}) == "valid");
    for (const auto* invalid : {"\xc0\x80", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2\x82", "\x80"})
    {
        expect_error("{}", {std::string(invalid)}, "字符串包含无效 UTF-8");
        expect_error(std::string("{!z}") + invalid, {}, "字符串包含无效 UTF-8");
    }
    std::string output;
    append_format_integer(output, std::numeric_limits<std::int64_t>::max());
    require(output == "9223372036854775807");
}

void eviction_and_lifetime()
{
    require(format("keep:{}", {std::string("first")}) == "keep:first");
    auto active = find_format_plan("keep:{}");
    require(active && active->parts.front().name.empty());
    for (std::size_t index = 0; index < format_cache_entries + 1; ++index)
    {
        const auto text = std::to_string(index) + ":{}";
        require(format(text, {std::int64_t{9}}) == std::to_string(index) + ":9");
    }
    require(!find_format_plan("keep:{}"));
    // 模拟当前调用持有计划时发生嵌套格式化并淘汰缓存；活动计划不含参数或借用指针。
    require(active->text == "keep:{}" && active->parts.front().literal == "keep:");
    const std::string large(format_cache_template_bytes + 1, 'x');
    require(format(large, {}).size() == large.size());
    require(!find_format_plan(large));
    std::string dense;
    for (int index = 0; index < 900; ++index)
    {
        dense += "{0}";
    }
    require(format(dense, {std::int64_t{1}}).size() == 900);
    // 密集计划即使未超过单模板门槛，也计入总字节上限。
    for (int index = 0; index < 4; ++index)
    {
        require(format(std::to_string(index) + dense, {std::int64_t{1}}).size() == 901);
    }
    require(!find_format_plan(dense));
    require(format("keep:{}", {std::string("second")}) == "keep:second");
}

} // namespace

int main()
{
    errors_and_values();
    eviction_and_lifetime();
    std::atomic<bool> valid = true;
    const auto worker = [&]
    {
        try
        {
            errors_and_values();
            eviction_and_lifetime();
            for (std::int64_t value = 0; value < 100; ++value)
            {
                require(format("{}:{}", {value, true}) == std::to_string(value) + ":true");
            }
        }
        catch (...)
        {
            valid = false;
        }
    };
    std::jthread first(worker);
    std::jthread second(worker);
    first.join();
    second.join();
    require(valid);
    auto& context = detail::current_runtime_context();
    const auto* previous_root = context.newest_handle;
    void* output = nullptr;
    require(txrt_format_begin(&output, std::numeric_limits<std::uint64_t>::max(), "", 0) != 0);
    require(std::string(txrt_last_error()) == "format 输出长度过大");
    txrt_str_release(output);
    require(context.newest_handle == previous_root);
    std::cout << "FORMAT_CACHE_OK\n";
}
