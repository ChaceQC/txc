#pragma once

#include "stdlib/native_gui/core/geometry.hpp"

#include <span>

namespace tx::ui
{
class pixel_buffer
{
public:
    pixel_buffer(unsigned width, unsigned height);
    unsigned width() const noexcept;
    unsigned height() const noexcept;
    std::span<const std::uint32_t> pixels() const noexcept;
    void clear(color value);
    void blend(int x, int y, color value, unsigned coverage = 255);
private:
    unsigned width_, height_;
    // 预乘 alpha 的 0xAARRGGBB，Windows DIB/X11 little-endian 下均可直接提交 BGRA。
    std::vector<std::uint32_t> pixels_;
};

class rasterizer
{
public:
    explicit rasterizer(pixel_buffer& target);
    void set_scale(double scale);
    void push_clip(rect bounds);
    void pop_clip();
    void fill(const path& shape, color value, bool even_odd = false);
    void fill_rect(rect bounds, color value);
    void fill_rounded_rect(rect bounds, double radius, color value);
    void fill_ellipse(rect bounds, color value);
    void line(point start, point end, double width, color value);
private:
    pixel_buffer& target_;
    std::vector<rect> clips_;
    double scale_ = 1;
};
}
