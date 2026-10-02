#include "stdlib/native_gui/core/raster.hpp"

#include <stdexcept>

namespace tx::ui
{
rasterizer::rasterizer(pixel_buffer& target) : target_(target)
{
    clips_.push_back({0, 0, double(target.width()), double(target.height())});
}

void rasterizer::set_scale(double scale)
{
    if (!std::isfinite(scale) || scale <= 0 || scale > 16 || clips_.size() != 1)
    {
        throw std::invalid_argument("缩放必须位于 (0,16] 且只能在裁剪栈为空时设置");
    }
    scale_ = scale;
}

void rasterizer::push_clip(rect bounds)
{
    if (clips_.size() >= 128 || !std::isfinite(bounds.x) || !std::isfinite(bounds.y) ||
        !std::isfinite(bounds.width) || !std::isfinite(bounds.height) || bounds.width < 0 || bounds.height < 0)
    {
        throw std::invalid_argument("裁剪矩形非法或裁剪栈超过 128 层");
    }
    bounds = {bounds.x * scale_, bounds.y * scale_, bounds.width * scale_, bounds.height * scale_};
    clips_.push_back(intersect(clips_.back(), bounds));
}

void rasterizer::pop_clip()
{
    if (clips_.size() <= 1)
    {
        throw std::invalid_argument("裁剪栈未配对");
    }
    clips_.pop_back();
}

void rasterizer::fill(const path& shape, color value, bool even_odd)
{
    struct crossing
    {
        double x;
        int winding;
    };
    std::vector<crossing> crossings;
    constexpr int samples = 4;
    double min_x = 1e7, min_y = 1e7, max_x = -1e7, max_y = -1e7;
    for (const auto& contour : shape.contours)
    {
        for (const auto point : contour.points)
        {
            if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
                std::abs(point.x) > 1e7 || std::abs(point.y) > 1e7)
            {
                throw std::invalid_argument("光栅路径坐标非法");
            }
            min_x = std::min(min_x, point.x * scale_);
            min_y = std::min(min_y, point.y * scale_);
            max_x = std::max(max_x, point.x * scale_);
            max_y = std::max(max_y, point.y * scale_);
        }
    }
    const auto clip = intersect(clips_.back(), {min_x, min_y, std::max(0.0, max_x - min_x),
        std::max(0.0, max_y - min_y)});
    if (clip.width <= 0 || clip.height <= 0)
    {
        return;
    }
    const int left = static_cast<int>(std::floor(clip.x)), right = static_cast<int>(std::ceil(clip.x + clip.width));
    const int top = static_cast<int>(std::floor(clip.y)), bottom = static_cast<int>(std::ceil(clip.y + clip.height));
    std::vector<unsigned> coverage(target_.width());
    // 每条子像素扫描线按非零环绕/奇偶规则积累覆盖率；半开边界避免共享顶点双计数。
    for (int y = top; y < bottom; ++y)
    {
        std::fill(coverage.begin(), coverage.end(), 0);
        for (int sample_y = 0; sample_y < samples; ++sample_y)
        {
            const double scan = y + (sample_y + 0.5) / samples;
            if (scan < clip.y || scan >= clip.y + clip.height)
            {
                continue;
            }
            crossings.clear();
            for (const auto& contour : shape.contours)
            {
                for (std::size_t i = 0; i < contour.points.size(); ++i)
                {
                    const auto first = contour.points[i], second = contour.points[(i + 1) % contour.points.size()];
                    const point a{first.x * scale_, first.y * scale_}, b{second.x * scale_, second.y * scale_};
                    if ((a.y <= scan && b.y > scan) || (b.y <= scan && a.y > scan))
                    {
                        crossings.push_back({a.x + (scan - a.y) * (b.x - a.x) / (b.y - a.y), b.y > a.y ? 1 : -1});
                    }
                }
            }
            std::sort(crossings.begin(), crossings.end(), [](const auto& a, const auto& b)
            {
                return a.x < b.x;
            });
            int winding = 0;
            for (std::size_t index = 0; index + 1 < crossings.size(); ++index)
            {
                winding += crossings[index].winding;
                if (even_odd ? (winding & 1) == 0 : winding == 0)
                {
                    continue;
                }
                const double begin = std::max(clip.x, crossings[index].x);
                const double end = std::min(clip.x + clip.width, crossings[index + 1].x);
                for (int x = std::max(left, static_cast<int>(std::floor(begin)));
                    x < std::min(right, static_cast<int>(std::ceil(end))); ++x)
                {
                    for (int sample_x = 0; sample_x < samples; ++sample_x)
                    {
                        const double position = x + (sample_x + 0.5) / samples;
                        coverage[x] += position >= begin && position < end;
                    }
                }
            }
        }
        for (int x = left; x < right; ++x)
        {
            if (coverage[x])
            {
                target_.blend(x, y, value, (coverage[x] * 255 + samples * samples / 2) / (samples * samples));
            }
        }
    }
}

void rasterizer::fill_rect(rect bounds, color value)
{
    if (bounds.width < 0 || bounds.height < 0)
    {
        throw std::invalid_argument("矩形尺寸不能为负");
    }
    path shape;
    shape.move_to({bounds.x, bounds.y});
    shape.line_to({bounds.x + bounds.width, bounds.y});
    shape.line_to({bounds.x + bounds.width, bounds.y + bounds.height});
    shape.line_to({bounds.x, bounds.y + bounds.height});
    fill(shape, value);
}

void rasterizer::fill_rounded_rect(rect bounds, double radius, color value)
{
    if (!std::isfinite(radius) || radius < 0 || bounds.width < 0 || bounds.height < 0)
    {
        throw std::invalid_argument("圆角矩形尺寸非法");
    }
    const double r = std::min({radius, bounds.width / 2, bounds.height / 2});
    const double x = bounds.x, y = bounds.y, right = x + bounds.width, bottom = y + bounds.height;
    path shape;
    shape.move_to({x + r, y});
    shape.line_to({right - r, y});
    shape.quadratic_to({right, y}, {right, y + r});
    shape.line_to({right, bottom - r});
    shape.quadratic_to({right, bottom}, {right - r, bottom});
    shape.line_to({x + r, bottom});
    shape.quadratic_to({x, bottom}, {x, bottom - r});
    shape.line_to({x, y + r});
    shape.quadratic_to({x, y}, {x + r, y});
    fill(shape, value);
}

void rasterizer::fill_ellipse(rect bounds, color value)
{
    const double x = bounds.x + bounds.width / 2, y = bounds.y + bounds.height / 2;
    const double rx = bounds.width / 2, ry = bounds.height / 2, k = 0.5522847498307936;
    if (rx < 0 || ry < 0)
    {
        throw std::invalid_argument("椭圆尺寸不能为负");
    }
    path shape;
    shape.move_to({x + rx, y});
    shape.cubic_to({x + rx, y + k * ry}, {x + k * rx, y + ry}, {x, y + ry});
    shape.cubic_to({x - k * rx, y + ry}, {x - rx, y + k * ry}, {x - rx, y});
    shape.cubic_to({x - rx, y - k * ry}, {x - k * rx, y - ry}, {x, y - ry});
    shape.cubic_to({x + k * rx, y - ry}, {x + rx, y - k * ry}, {x + rx, y});
    fill(shape, value);
}

void rasterizer::line(point start, point end, double width, color value)
{
    if (!std::isfinite(width) || width <= 0)
    {
        throw std::invalid_argument("线宽必须为正的有限数");
    }
    const double length = std::hypot(end.x - start.x, end.y - start.y), radius = width / 2;
    if (length < 1e-9)
    {
        fill_ellipse({start.x - radius, start.y - radius, width, width}, value);
        return;
    }
    const double dx = (end.y - start.y) / length * radius, dy = (start.x - end.x) / length * radius;
    path shape;
    shape.move_to({start.x + dx, start.y + dy});
    shape.line_to({end.x + dx, end.y + dy});
    shape.line_to({end.x - dx, end.y - dy});
    shape.line_to({start.x - dx, start.y - dy});
    fill(shape, value);
}
}
