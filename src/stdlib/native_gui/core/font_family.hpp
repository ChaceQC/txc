#pragma once

#include "stdlib/native_gui/core/font.hpp"
#include <memory>

namespace tx::ui
{
class font_family
{
public:
    explicit font_family(const font_face& primary);
    explicit font_family(std::shared_ptr<const font_face> primary);
    void add(std::shared_ptr<const font_face> face);
    const font_face& primary() const;
    const font_face& select(std::u32string_view cluster) const;
    double line_height(double size) const;
    double ascender(double size) const;
private:
    std::vector<const font_face*> faces_;
    std::vector<std::shared_ptr<const font_face>> owned_;
};

font_family system_font_family(const std::filesystem::path& primary);
}
