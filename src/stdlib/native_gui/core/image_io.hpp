#pragma once

#include "stdlib/native_gui/core/raster.hpp"

#include <filesystem>

namespace tx::ui
{
void save_bitmap(const pixel_buffer& image, const std::filesystem::path& file);
}
