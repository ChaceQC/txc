#pragma once

#include <vector>

namespace tx_generated::gui
{

enum class length_mode
{
    fixed, automatic, stretch
};

struct length
{
    length_mode mode = length_mode::automatic;
    double value = 0;
};

struct axis_item
{
    length policy;
    double natural = 0;
    double minimum = 0;
    double maximum = 16384;
};

struct extent
{
    double width = 0;
    double height = 0;
};

struct bounds
{
    double x = 0;
    double y = 0;
    double width = 0;
    double height = 0;
};

enum class layout_mode
{
    column, row, grid
};

enum class alignment
{
    start, center, end, stretch
};

std::vector<double> allocate_axis(const std::vector<axis_item>& items, double available);
double preferred(const axis_item& item);

} // namespace tx_generated::gui
