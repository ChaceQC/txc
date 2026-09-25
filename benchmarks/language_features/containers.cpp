#include "compare.hpp"

#include <any>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace
{

using dynamic_array = std::vector<std::any>;
using dict_key = std::variant<std::int64_t, std::string, bool>;
using dynamic_dict = std::unordered_map<dict_key, std::any>;

} // namespace

void bench_array_destructure()
{
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 20000; ++i)
    {
        dynamic_array pair{i, i + 1};
        const std::any left = pair[0];
        const std::any right = pair[1];
        pair[1] = std::any_cast<std::int64_t>(right) + 1;
        for (const auto& item : pair)
        {
            checksum += std::any_cast<std::int64_t>(item);
        }
    }
    report("array_destructure", started, checksum);
}

void bench_array_padded()
{
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 50000; ++i)
    {
        dynamic_array values(4);
        values[0] = i;
        values[1] = i + 1;
        if (!values[3].has_value())
        {
            checksum += static_cast<std::int64_t>(values.size());
        }
        values[2] = i;
        checksum += std::any_cast<std::int64_t>(values[2]);
    }
    report("array_padded", started, checksum);
}

void bench_dict_iteration()
{
    dynamic_dict values;
    values.emplace(dict_key{std::int64_t{1}}, std::int64_t{1});
    values.emplace(dict_key{std::string{"two"}}, std::int64_t{2});
    values.emplace(dict_key{true}, std::int64_t{3});
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 100000; ++i)
    {
        values[dict_key{std::string{"two"}}] = i;
        for (const auto& entry : values)
        {
            checksum += 1;
        }
        checksum += std::any_cast<std::int64_t>(
            values.at(dict_key{std::string{"two"}}));
    }
    report("dict_iteration", started, checksum);
}
