#include "stdlib/native_gui/core/image_io.hpp"

#include <fstream>
#include <stdexcept>

namespace tx::ui
{
void save_bitmap(const pixel_buffer& image, const std::filesystem::path& file)
{
    std::ofstream output(file, std::ios::binary);
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
        throw std::runtime_error("无法保存 BMP 图像");
    }
}
}
