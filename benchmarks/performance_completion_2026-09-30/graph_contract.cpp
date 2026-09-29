// TX 与独立 C++ 对照都使用任意共享/循环边、身份表复制及外部引用计数图扫描。
#include "backend/cpp/deep_copy.hpp"
#include "backend/cpp/cycle_gc.hpp"
#include "stdlib/array.hpp"

#include <windows.h>
#include <psapi.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <variant>
#include <vector>

namespace
{

using clock_type = std::chrono::steady_clock;
using node_value = std::variant<std::int64_t, std::shared_ptr<struct node>, std::shared_ptr<int>>;
struct node
{
    std::vector<node_value> values;
};
std::vector<std::weak_ptr<node>> registry;
std::vector<std::weak_ptr<int>> reclaimed;

void watch_tx(tx_generated::tx_array current)
{
    for (int index = 0; index < 128; ++index)
    {
        auto next = std::any_cast<tx_generated::tx_array>(std::as_const(current)[1]);
        auto token = std::make_shared<int>(index);
        reclaimed.push_back(token);
        current.push_back(std::move(token));
        current = std::move(next);
    }
}

void watch_cpp(std::shared_ptr<node> current)
{
    for (int index = 0; index < 128; ++index)
    {
        auto next = std::get<std::shared_ptr<node>>(current->values[1]);
        auto token = std::make_shared<int>(index);
        reclaimed.push_back(token);
        current->values.emplace_back(std::move(token));
        current = std::move(next);
    }
}

std::shared_ptr<node> make_node()
{
    auto result = std::make_shared<node>();
    registry.push_back(result);
    return result;
}

std::shared_ptr<node> copy_node(const std::shared_ptr<node>& source,
    std::unordered_map<const node*, std::shared_ptr<node>>& copies)
{
    if (const auto found = copies.find(source.get()); found != copies.end())
    {
        return found->second;
    }
    auto result = make_node();
    copies.emplace(source.get(), result);
    result->values.reserve(source->values.size());
    for (const auto& value : source->values)
    {
        if (const auto* scalar = std::get_if<std::int64_t>(&value))
        {
            result->values.emplace_back(*scalar);
        }
        else
        {
            result->values.emplace_back(copy_node(std::get<std::shared_ptr<node>>(value), copies));
        }
    }
    return result;
}

void collect_nodes()
{
    std::vector<std::shared_ptr<node>> nodes;
    for (const auto& weak : registry)
    {
        if (auto value = weak.lock())
        {
            nodes.push_back(std::move(value));
        }
    }
    std::unordered_map<const node*, std::size_t> positions;
    std::vector<std::int64_t> refs;
    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        positions.emplace(nodes[index].get(), index);
        refs.push_back(nodes[index].use_count() - 1);
    }
    std::vector<std::vector<std::size_t>> edges(nodes.size());
    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        for (const auto& value : nodes[index]->values)
        {
            if (const auto* child = std::get_if<std::shared_ptr<node>>(&value))
            {
                const auto target = positions.at(child->get());
                edges[index].push_back(target);
                --refs[target];
            }
        }
    }
    std::vector<bool> reached(nodes.size(), false);
    std::vector<std::size_t> pending;
    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        if (refs[index] > 0)
        {
            reached[index] = true;
            pending.push_back(index);
        }
    }
    while (!pending.empty())
    {
        const auto index = pending.back();
        pending.pop_back();
        for (const auto target : edges[index])
        {
            if (!reached[target])
            {
                reached[target] = true;
                pending.push_back(target);
            }
        }
    }
    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        if (!reached[index])
        {
            nodes[index]->values.clear();
        }
    }
    nodes.clear();
    std::erase_if(registry, [](const auto& value)
    {
        return value.expired();
    });
}

std::int64_t micros(clock_type::time_point start)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(clock_type::now() - start).count();
}

std::int64_t tx_copy_round()
{
    std::vector<tx_generated::tx_array> nodes(128);
    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        nodes[index].push_back(static_cast<std::int64_t>(index));
        nodes[index].push_back(nodes[(index + 1) % nodes.size()]);
        nodes[index].push_back(nodes[(index * 7) % nodes.size()]);
    }
    const auto start = clock_type::now();
    const auto copied = tx_generated::deep_copy_value(nodes.front());
    const auto elapsed = micros(start);
    const auto& root = std::any_cast<const tx_generated::tx_array&>(copied);
    const auto& self = std::any_cast<const tx_generated::tx_array&>(root[2]);
    if (self.identity() != root.identity() || root.identity() == nodes.front().identity())
    {
        throw std::runtime_error("TX 图复制身份错误");
    }
    watch_tx(nodes.front());
    watch_tx(root);
    return elapsed;
}

std::int64_t cpp_copy_round()
{
    std::vector<std::shared_ptr<node>> nodes;
    for (std::size_t index = 0; index < 128; ++index)
    {
        nodes.push_back(make_node());
    }
    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        nodes[index]->values.emplace_back(static_cast<std::int64_t>(index));
        nodes[index]->values.emplace_back(nodes[(index + 1) % nodes.size()]);
        nodes[index]->values.emplace_back(nodes[(index * 7) % nodes.size()]);
    }
    const auto start = clock_type::now();
    std::unordered_map<const node*, std::shared_ptr<node>> copies;
    const auto copied = copy_node(nodes.front(), copies);
    const auto elapsed = micros(start);
    if (std::get<std::shared_ptr<node>>(copied->values[2]) != copied || copied == nodes.front())
    {
        throw std::runtime_error("C++ 图复制身份错误");
    }
    watch_cpp(nodes.front());
    watch_cpp(copied);
    return elapsed;
}

} // namespace

int main(int argc, char**)
{
    const bool reference = argc > 1;
    std::int64_t copies = 0;
    std::int64_t collections = 0;
    std::int64_t maximum_pause = 0;
    std::int64_t collected = 0;
    for (int round = 0; round < 100; ++round)
    {
        copies += reference ? cpp_copy_round() : tx_copy_round();
        const auto start = clock_type::now();
        if (reference)
        {
            collect_nodes();
        }
        else
        {
            tx_generated::collect_cycles();
        }
        const auto pause = micros(start);
        if (reclaimed.size() != 256 || !std::all_of(reclaimed.begin(), reclaimed.end(), [](const auto& weak)
        {
            return weak.expired();
        }))
        {
            throw std::runtime_error("图回收没有释放全部 256 个节点");
        }
        collected += reclaimed.size();
        reclaimed.clear();
        collections += pause;
        maximum_pause = std::max(maximum_pause, pause);
    }
    PROCESS_MEMORY_COUNTERS memory{};
    GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory));
    std::cout << "graph_copy\n" << copies << "\n12800\ngraph_gc\n" << collections << '\n' << collected << '\n';
    std::cerr << "{\"peak_working_set_bytes\":" << memory.PeakWorkingSetSize
              << ",\"peak_pagefile_bytes\":" << memory.PeakPagefileUsage
              << ",\"maximum_gc_pause_us\":" << maximum_pause << "}\n";
}
