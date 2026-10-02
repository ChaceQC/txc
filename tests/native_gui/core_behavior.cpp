#include "stdlib/native_gui/core/font.hpp"
#include "stdlib/native_gui/core/raster.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>

using namespace tx::ui;

namespace
{
template<class operation>
void reject(operation run)
{
    bool rejected = false;
    try
    {
        run();
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    assert(rejected);
}

void write_bitmap(const pixel_buffer& image, const std::filesystem::path& path)
{
    std::ofstream output(path, std::ios::binary);
    const auto word = [&](std::uint32_t value, unsigned bytes = 4)
    {
        for (unsigned index = 0; index < bytes; ++index)
        {
            output.put(static_cast<char>(value >> (index * 8)));
        }
    };
    word(0x4d42, 2);
    word(54 + image.width() * image.height() * 4);
    word(0);
    word(54);
    word(40);
    word(image.width());
    word(0u - image.height());
    word(1, 2);
    word(32, 2);
    word(0);
    word(image.width() * image.height() * 4);
    word(3780);
    word(3780);
    word(0);
    word(0);
    for (const auto pixel : image.pixels())
    {
        word(pixel);
    }
    if (!output)
    {
        throw std::runtime_error("无法保存渲染检查图像");
    }
}

void verify_raster()
{
    pixel_buffer pixels(32, 32);
    pixels.clear({0, 0, 0, 0});
    pixels.blend(0, 0, {255, 0, 0, 128});
    assert(pixels.pixels()[0] == 0x80800000);
    pixels.blend(0, 0, {0, 0, 255, 128});
    assert(pixels.pixels()[0] == 0xc0400080);
    rasterizer painter(pixels);
    pixels.clear({255, 255, 255, 255});
    painter.push_clip({4, 4, 12, 12});
    painter.fill_rect({0, 0, 32, 32}, {0, 0, 0, 255});
    painter.pop_clip();
    assert(pixels.pixels()[4 * 32 + 4] == 0xff000000);
    assert(pixels.pixels()[3 * 32 + 4] == 0xffffffff);
    painter.push_clip({1e100, 1e100, 5, 5});
    painter.fill_rect({0, 0, 32, 32}, {255, 0, 0, 255});
    painter.pop_clip();
    path hole;
    hole.move_to({0, 0});
    hole.line_to({30, 0});
    hole.line_to({30, 30});
    hole.line_to({0, 30});
    hole.move_to({10, 10});
    hole.line_to({20, 10});
    hole.line_to({20, 20});
    hole.line_to({10, 20});
    pixels.clear({255, 255, 255, 255});
    painter.fill(hole, {0, 0, 0, 255}, true);
    assert(pixels.pixels()[15 * 32 + 15] == 0xffffffff);
    painter.fill(hole, {0, 0, 0, 255});
    assert(pixels.pixels()[15 * 32 + 15] == 0xff000000);
    path reversal;
    reversal.move_to({0, 0});
    reversal.quadratic_to({40, 0}, {10, 0});
    assert(std::any_of(reversal.contours[0].points.begin(), reversal.contours[0].points.end(),
        [](point value)
        {
            return value.x > 20;
        }));
    reject([&]
    {
        painter.pop_clip();
    });
    reject([&]
    {
        painter.fill_rect({0, 0, -1, 2}, {});
    });
}

void text(rasterizer& painter, const font_face& font, std::u32string_view content, point baseline,
    double size, color color)
{
    for (const auto scalar : content)
    {
        const auto glyph = font.glyph(scalar);
        painter.fill(font.outline(glyph, size, baseline), color);
        baseline.x += font.advance(glyph, size);
    }
}
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "usage: core_behavior font.ttf output.bmp\n";
        return 2;
    }
    verify_raster();
    reject([]
    {
        font_face invalid(std::vector<std::uint8_t>(16));
    });
    const auto font = font_face::load(argv[1]);
    assert(font.glyph(U'A') != 0 && font.glyph(U'中') != 0);
    assert(font.advance(font.glyph(U'A'), 24) > 0);
    reject([&]
    {
        font.glyph(char32_t(0x110000));
    });
    pixel_buffer pixels(780, 420);
    pixels.clear({236, 241, 250, 255});
    rasterizer painter(pixels);
    painter.fill_rounded_rect({24, 24, 732, 372}, 16, {255, 255, 255, 255});
    text(painter, font, U"TX 自研绘图与字体引擎", {48, 80}, 30, {30, 45, 70, 255});
    text(painter, font, U"不调用系统绘图、字体解析或文字排版", {48, 124}, 20, {80, 95, 120, 255});
    painter.fill_rounded_rect({48, 160, 190, 54}, 10, {48, 90, 180, 255});
    text(painter, font, U"自行绘制按钮", {68, 196}, 23, {255, 255, 255, 255});
    painter.fill_ellipse({280, 162, 50, 50}, {50, 160, 125, 255});
    painter.line({292, 186}, {304, 198}, 3, {255, 255, 255, 255});
    painter.line({304, 198}, {320, 176}, 3, {255, 255, 255, 255});
    text(painter, font, U"TrueType / TTC  ·  ABC xyz 0123", {48, 266}, 24, {30, 45, 70, 255});
    text(painter, font, U"中文：窗口、布局、输入与像素提交", {48, 310}, 24, {30, 45, 70, 255});
    text(painter, font, U"é Å ñ ç  —  字形轮廓与二次曲线", {48, 356}, 22, {80, 95, 120, 255});
    write_bitmap(pixels, argv[2]);
    std::cout << "self renderer / TrueType / TTC / cmap / outline: PASS\n";
}
