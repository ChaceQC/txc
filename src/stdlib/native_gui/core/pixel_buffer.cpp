#include "stdlib/native_gui/core/raster.hpp"

#include <stdexcept>

namespace tx::ui
{
namespace
{
unsigned multiply(unsigned channel, unsigned alpha)
{
    return (channel * alpha + 127) / 255;
}

std::uint32_t premultiply(color value)
{
    return (std::uint32_t(value.alpha) << 24) | (multiply(value.red, value.alpha) << 16) |
        (multiply(value.green, value.alpha) << 8) | multiply(value.blue, value.alpha);
}
}

pixel_buffer::pixel_buffer(unsigned width, unsigned height) : width_(width), height_(height)
{
    if (!width || !height || width > 16384 || height > 16384 ||
        std::uint64_t(width) * height > 64 * 1024 * 1024)
    {
        throw std::length_error("像素缓冲尺寸非法或超过 256 MiB");
    }
    pixels_.resize(std::size_t(width) * height);
}

unsigned pixel_buffer::width() const noexcept
{
    return width_;
}

unsigned pixel_buffer::height() const noexcept
{
    return height_;
}

std::span<const std::uint32_t> pixel_buffer::pixels() const noexcept
{
    return pixels_;
}

void pixel_buffer::clear(color value)
{
    std::fill(pixels_.begin(), pixels_.end(), premultiply(value));
}

void pixel_buffer::blend(int x, int y, color value, unsigned coverage)
{
    if (x < 0 || y < 0 || static_cast<unsigned>(x) >= width_ || static_cast<unsigned>(y) >= height_)
    {
        return;
    }
    if (coverage > 255)
    {
        throw std::invalid_argument("像素覆盖率超过 255");
    }
    auto& target = pixels_[std::size_t(y) * width_ + x];
    const unsigned alpha = multiply(value.alpha, coverage), inverse = 255 - alpha;
    const unsigned a = alpha + multiply(target >> 24, inverse);
    const unsigned r = multiply(value.red, alpha) + multiply((target >> 16) & 255, inverse);
    const unsigned g = multiply(value.green, alpha) + multiply((target >> 8) & 255, inverse);
    const unsigned b = multiply(value.blue, alpha) + multiply(target & 255, inverse);
    target = (a << 24) | (r << 16) | (g << 8) | b;
}
}
