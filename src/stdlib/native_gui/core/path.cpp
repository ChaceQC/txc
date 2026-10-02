#include "stdlib/native_gui/core/geometry.hpp"

#include <stdexcept>

namespace tx::ui
{
namespace
{
void validate(point value)
{
    if (!std::isfinite(value.x) || !std::isfinite(value.y) ||
        std::abs(value.x) > 1e7 || std::abs(value.y) > 1e7)
    {
        throw std::invalid_argument("路径坐标必须有限且绝对值不超过一千万");
    }
}

point midpoint(point a, point b)
{
    return {(a.x + b.x) / 2, (a.y + b.y) / 2};
}

double deviation(point control, point first, point last)
{
    const double dx = last.x - first.x, dy = last.y - first.y;
    const double length_squared = dx * dx + dy * dy;
    if (length_squared < 1e-18)
    {
        return std::hypot(control.x - first.x, control.y - first.y);
    }
    // 到有限线段的距离同时覆盖共线回折；无限直线距离会把越过端点的曲线错误压平。
    const double projection = std::clamp(((control.x - first.x) * dx +
        (control.y - first.y) * dy) / length_squared, 0.0, 1.0);
    return std::hypot(control.x - first.x - projection * dx, control.y - first.y - projection * dy);
}

void quadratic(path& output, point start, point control, point end, double tolerance, unsigned depth)
{
    if (depth == 16 || deviation(control, start, end) <= tolerance)
    {
        output.line_to(end);
        return;
    }
    const auto a = midpoint(start, control), b = midpoint(control, end), middle = midpoint(a, b);
    quadratic(output, start, a, middle, tolerance, depth + 1);
    quadratic(output, middle, b, end, tolerance, depth + 1);
}

void cubic(path& output, point start, point first, point second, point end, double tolerance, unsigned depth)
{
    if (depth == 16 || std::max(deviation(first, start, end), deviation(second, start, end)) <= tolerance)
    {
        output.line_to(end);
        return;
    }
    const auto a = midpoint(start, first), b = midpoint(first, second), c = midpoint(second, end);
    const auto d = midpoint(a, b), e = midpoint(b, c), middle = midpoint(d, e);
    cubic(output, start, a, d, middle, tolerance, depth + 1);
    cubic(output, middle, e, c, end, tolerance, depth + 1);
}
}

void path::move_to(point value)
{
    validate(value);
    if (contours.size() >= 65536)
    {
        throw std::length_error("路径轮廓数量超限");
    }
    contours.push_back({{value}});
}

void path::line_to(point value)
{
    validate(value);
    if (contours.empty() || contours.back().points.size() >= 262144)
    {
        throw std::length_error("路径缺少起点或轮廓点数量超限");
    }
    contours.back().points.push_back(value);
}

void path::quadratic_to(point control, point end, double tolerance)
{
    validate(control);
    validate(end);
    if (contours.empty() || !std::isfinite(tolerance) || tolerance <= 0)
    {
        throw std::invalid_argument("曲线缺少起点或精度非法");
    }
    quadratic(*this, contours.back().points.back(), control, end, tolerance, 0);
}

void path::cubic_to(point first, point second, point end, double tolerance)
{
    validate(first);
    validate(second);
    validate(end);
    if (contours.empty() || !std::isfinite(tolerance) || tolerance <= 0)
    {
        throw std::invalid_argument("曲线缺少起点或精度非法");
    }
    cubic(*this, contours.back().points.back(), first, second, end, tolerance, 0);
}
}
