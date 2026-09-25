#include "compare.hpp"

#include <algorithm>
#include <any>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{

using dynamic_array = std::vector<std::any>;
using array_ref = std::shared_ptr<dynamic_array>;
using dynamic_dict = std::unordered_map<std::string, array_ref>;

struct record
{
    std::shared_ptr<dynamic_dict> items;
};

record copy_record(const record& source)
{
    auto copied_items = std::make_shared<dynamic_dict>();
    for (const auto& [key, values] : *source.items)
    {
        copied_items->emplace(key, std::make_shared<dynamic_array>(*values));
    }
    return record{std::move(copied_items)};
}

struct cycle_array;
using cycle_ref = std::shared_ptr<cycle_array>;

struct cycle_array
{
    std::array<std::any, 2> items;
};

cycle_ref copy_cycle_graph(const cycle_ref& source,
                           std::unordered_map<const cycle_array*, cycle_ref>& seen)
{
    if (const auto found = seen.find(source.get()); found != seen.end())
    {
        return found->second;
    }
    auto copied = std::make_shared<cycle_array>();
    seen.emplace(source.get(), copied);
    for (std::size_t index = 0; index < source->items.size(); ++index)
    {
        const auto& item = source->items[index];
        if (item.type() == typeid(cycle_ref))
        {
            copied->items[index] = copy_cycle_graph(
                std::any_cast<const cycle_ref&>(item), seen);
        }
        else
        {
            copied->items[index] = item;
        }
    }
    return copied;
}

class tracked
{
public:
    explicit tracked(array_ref totals) : totals_(std::move(totals))
    {
    }

    ~tracked()
    {
        totals_->at(0) = std::any_cast<std::int64_t>(totals_->at(0)) + 1;
    }

private:
    array_ref totals_;
};

class cycle_node
{
public:
    explicit cycle_node(array_ref totals) : totals_(std::move(totals))
    {
    }

    ~cycle_node()
    {
        totals_->at(0) = std::any_cast<std::int64_t>(totals_->at(0)) + 1;
    }

    std::shared_ptr<cycle_node> next;

private:
    array_ref totals_;
};

class cycle_registry
{
public:
    void add(const std::shared_ptr<cycle_node>& value)
    {
        nodes_.push_back(value);
        ++allocations_;
    }

    void note_allocation()
    {
        ++allocations_;
    }

    void safepoint()
    {
        const auto threshold = std::max<std::size_t>(64, nodes_.size() / 2);
        if (!nodes_.empty() && allocations_ >= threshold)
        {
            collect();
        }
    }

private:
    void collect()
    {
        // 此对照只识别基准创建的单节点自环；TX 回收器处理任意对象图。
        for (const auto& weak : nodes_)
        {
            auto value = weak.lock();
            if (value && value.use_count() == 2 &&
                value->next.get() == value.get())
            {
                value->next.reset();
            }
        }
        std::erase_if(nodes_, [](const auto& weak)
        {
            return weak.expired();
        });
        allocations_ = 0;
    }

    std::vector<std::weak_ptr<cycle_node>> nodes_;
    std::size_t allocations_ = 0;
};

void make_cycle(cycle_registry& registry, const array_ref& totals)
{
    auto value = std::make_shared<cycle_node>(totals);
    value->next = value;
    registry.add(value);
}

} // namespace

void bench_deep_copy()
{
    auto numbers = std::make_shared<dynamic_array>(
        dynamic_array{std::int64_t{1}, std::int64_t{2}, std::int64_t{3}});
    auto items = std::make_shared<dynamic_dict>();
    items->emplace("numbers", numbers);
    record source{items};
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 10000; ++i)
    {
        record copied = copy_record(source);
        copied.items->at("numbers")->at(0) = i;
        checksum += std::any_cast<std::int64_t>(
            copied.items->at("numbers")->at(0));
    }
    checksum += std::any_cast<std::int64_t>(source.items->at("numbers")->at(0));
    report("deep_copy", started, checksum);
}

void bench_copy_cycle()
{
    auto source = std::make_shared<cycle_array>();
    source->items[0] = source;
    source->items[1] = std::int64_t{7};
    std::int64_t checksum = 0;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 10000; ++i)
    {
        std::unordered_map<const cycle_array*, cycle_ref> seen;
        auto copied = copy_cycle_graph(source, seen);
        auto self = std::any_cast<cycle_ref>(copied->items[0]);
        self->items[1] = i;
        checksum += std::any_cast<std::int64_t>(copied->items[1]);
        copied->items[0].reset();
    }
    checksum += std::any_cast<std::int64_t>(source->items[1]);
    report("copy_cycle", started, checksum);
    source->items[0].reset();
}

void bench_deinit()
{
    auto totals = std::make_shared<dynamic_array>(1, std::int64_t{0});
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 20000; ++i)
    {
        auto item = std::make_shared<tracked>(totals);
    }
    report("deinit", started, std::any_cast<std::int64_t>(totals->at(0)));
}

void bench_cycle_gc()
{
    auto totals = std::make_shared<dynamic_array>(1, std::int64_t{0});
    cycle_registry registry;
    const auto started = clock_type::now();
    for (std::int64_t i = 1; i <= 1000; ++i)
    {
        make_cycle(registry, totals);
        registry.safepoint();
    }
    std::int64_t attempts = 0;
    while (std::any_cast<std::int64_t>(totals->at(0)) < 1000 &&
           attempts < 4096)
    {
        auto padding = std::make_shared<dynamic_array>();
        registry.note_allocation();
        registry.safepoint();
        ++attempts;
    }
    report("cycle_gc", started, std::any_cast<std::int64_t>(totals->at(0)));
}
