#include "stdlib/native_gui/core/opentype_internal.hpp"

namespace tx::ui
{
std::vector<double> ligature_carets(font_reader gdef, std::uint16_t glyph, double scale)
{
    if (!gdef.size() || !gdef.u16(8))
    {
        return {};
    }
    const auto list = ot::subtable(gdef, gdef.u16(8));
    const auto covered = ot::coverage(ot::subtable(list, list.u16(0)), glyph);
    if (covered < 0)
    {
        return {};
    }
    if (covered >= list.u16(2))
    {
        throw std::invalid_argument("GDEF 连字光标覆盖索引越界");
    }
    const auto ligature = ot::subtable(list, list.u16(4 + covered * 2));
    const auto count = ligature.u16(0);
    ligature.slice(2, std::size_t(count) * 2);
    std::vector<double> result;
    for (unsigned i = 0; i < count; ++i)
    {
        const auto caret = ot::subtable(ligature, ligature.u16(2 + i * 2));
        const auto format = caret.u16(0);
        if (format == 2)
        {
            // 轮廓点型光标需要拟合后的点；当前无 hinting，使用均分策略。
            caret.u16(2);
            return {};
        }
        if (format != 1 && format != 3)
        {
            throw std::invalid_argument("GDEF CaretValue 格式非法");
        }
        if (format == 3 && caret.u16(4))
        {
            // 本轮布局保留未 hint 的设计坐标，不应用像素网格光标微调。
            const auto device = ot::subtable(caret, caret.u16(4));
            device.slice(0, 6);
        }
        result.push_back(caret.i16(2) * scale);
    }
    return result;
}
}
