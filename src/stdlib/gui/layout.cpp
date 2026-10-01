#include "stdlib/gui/layout.hpp"

#include <algorithm>
#include <numeric>

namespace tx_generated::gui
{

double preferred(const axis_item& item)
{
    return std::clamp(item.policy.mode == length_mode::fixed ? item.policy.value :
        item.natural, item.minimum, item.maximum);
}

std::vector<double> allocate_axis(const std::vector<axis_item>& items, double available)
{
    std::vector<double> sizes;
    sizes.reserve(items.size());
    for (const auto& item : items)
    {
        sizes.push_back(item.policy.mode == length_mode::stretch ? item.minimum : preferred(item));
    }
    // 每轮至少一个轨道到达 min/max，最多 N+1 轮；固定轨道不参与收缩。
    for (std::size_t pass = 0; pass <= items.size(); ++pass)
    {
        const auto remaining = std::max(0.0, available) -
            std::accumulate(sizes.begin(), sizes.end(), 0.0);
        if (remaining > -0.000001 && remaining < 0.000001)
        {
            break;
        }
        double weight = 0;
        for (std::size_t index = 0; index < items.size(); ++index)
        {
            const auto& item = items[index];
            if (remaining > 0 && item.policy.mode == length_mode::stretch &&
                sizes[index] < item.maximum)
            {
                weight += item.policy.value;
            }
            else if (remaining < 0 && item.policy.mode != length_mode::fixed &&
                sizes[index] > item.minimum)
            {
                weight += sizes[index] - item.minimum;
            }
        }
        if (weight == 0)
        {
            break;
        }
        for (std::size_t index = 0; index < items.size(); ++index)
        {
            const auto& item = items[index];
            if (remaining > 0 && item.policy.mode == length_mode::stretch &&
                sizes[index] < item.maximum)
            {
                sizes[index] = std::min(item.maximum,
                    sizes[index] + remaining * item.policy.value / weight);
            }
            else if (remaining < 0 && item.policy.mode != length_mode::fixed)
            {
                sizes[index] = std::max(item.minimum,
                    sizes[index] + remaining * (sizes[index] - item.minimum) / weight);
            }
        }
    }
    return sizes;
}

} // namespace tx_generated::gui
