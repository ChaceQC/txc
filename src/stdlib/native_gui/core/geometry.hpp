#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace tx::ui
{
struct point
{
    double x = 0, y = 0;
};

struct rect
{
    double x = 0, y = 0, width = 0, height = 0;
    bool contains(point value) const noexcept
    {
        return value.x >= x && value.y >= y && value.x < x + width && value.y < y + height;
    }
};

inline rect intersect(rect a, rect b) noexcept
{
    const double x = std::max(a.x, b.x), y = std::max(a.y, b.y);
    return {x, y, std::max(0.0, std::min(a.x + a.width, b.x + b.width) - x),
        std::max(0.0, std::min(a.y + a.height, b.y + b.height) - y)};
}

struct color
{
    std::uint8_t red = 0, green = 0, blue = 0, alpha = 255;
};

struct contour
{
    std::vector<point> points;
};

struct path
{
    std::vector<contour> contours;
    void move_to(point value);
    void line_to(point value);
    void quadratic_to(point control, point end, double tolerance = 0.2);
    void cubic_to(point first, point second, point end, double tolerance = 0.2);
};
}
