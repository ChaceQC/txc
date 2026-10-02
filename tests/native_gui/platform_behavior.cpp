#include "stdlib/native_gui/platform/window.hpp"
#include "stdlib/native_gui/core/font.hpp"
#include "stdlib/native_gui/core/text_layout.hpp"

#include <chrono>
#include <iostream>

using namespace tx::ui;

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    const auto font = font_face::load(argv[1]);
    const auto window = create_platform_window("TX 自研跨平台像素窗口", 640, 360);
    window->show();
    bool presented = false;
    const auto started = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - started < std::chrono::seconds(3))
    {
        const auto event = window->next_event(50);
        if (event && (event->kind == event_kind::redraw || event->kind == event_kind::resized))
        {
            const auto width = event->width ? event->width : static_cast<unsigned>(640 * window->scale());
            const auto height = event->height ? event->height : static_cast<unsigned>(360 * window->scale());
            pixel_buffer pixels(width, height);
            pixels.clear({235, 242, 252, 255});
            rasterizer renderer(pixels);
            renderer.set_scale(window->scale());
            renderer.fill_rounded_rect({24, 24, 592, 312}, 16, {255, 255, 255, 255});
            const text_layout text(font, U"TX 自研跨平台窗口\n只有系统输入和像素提交\n字体、路径与抗锯齿均自行实现", 24, 540, true);
            text.draw(renderer, {48, 60}, {25, 45, 75, 255});
            renderer.fill_rounded_rect({48, 220, 200, 52}, 10, {48, 90, 180, 255});
            const text_layout button(font, U"原生像素提交", 23, 190, false);
            button.draw(renderer, {72, 229}, {255, 255, 255, 255});
            window->present(pixels);
            presented = true;
        }
        if (event && event->kind == event_kind::close_requested)
        {
            break;
        }
    }
    window->close();
    if (!presented || window->is_open())
    {
        return 1;
    }
    std::cout << "native platform / self-rendered pixels / window lifetime PASS\n";
}
