#include "stdlib/native_gui/core/font.hpp"

#include <fstream>
#include <stdexcept>

namespace tx::ui
{
font_reader::font_reader(std::span<const std::uint8_t> data) : data_(data)
{
}

std::span<const std::uint8_t> font_reader::slice(std::size_t offset, std::size_t length) const
{
    if (offset > data_.size() || length > data_.size() - offset)
    {
        throw std::invalid_argument("字体表数据截断或偏移越界");
    }
    return data_.subspan(offset, length);
}

std::size_t font_reader::size() const noexcept
{
    return data_.size();
}

std::uint8_t font_reader::u8(std::size_t offset) const
{
    return slice(offset, 1)[0];
}

std::uint16_t font_reader::u16(std::size_t offset) const
{
    const auto value = slice(offset, 2);
    return static_cast<std::uint16_t>((value[0] << 8) | value[1]);
}

std::int16_t font_reader::i16(std::size_t offset) const
{
    const auto value = u16(offset);
    return static_cast<std::int16_t>(value < 0x8000 ? int(value) : int(value) - 0x10000);
}

std::uint32_t font_reader::u32(std::size_t offset) const
{
    const auto value = slice(offset, 4);
    return (std::uint32_t(value[0]) << 24) | (std::uint32_t(value[1]) << 16) |
        (std::uint32_t(value[2]) << 8) | value[3];
}

namespace
{
std::uint32_t tag(std::string_view name)
{
    if (name.size() != 4)
    {
        throw std::invalid_argument("字体表名必须为四字节");
    }
    return (std::uint32_t(static_cast<unsigned char>(name[0])) << 24) |
        (std::uint32_t(static_cast<unsigned char>(name[1])) << 16) |
        (std::uint32_t(static_cast<unsigned char>(name[2])) << 8) |
        static_cast<unsigned char>(name[3]);
}
}

font_reader font_face::table(std::string_view name) const
{
    const auto found = tables_.find(tag(name));
    if (found == tables_.end())
    {
        throw std::invalid_argument("字体缺少必要的 TrueType 表：" + std::string(name));
    }
    return font_reader(font_reader(data_).slice(found->second.offset, found->second.length));
}

font_reader font_face::layout_table(std::string_view name) const
{
    const auto found = tables_.find(tag(name));
    return found == tables_.end() ? font_reader({}) :
        font_reader(font_reader(data_).slice(found->second.offset, found->second.length));
}

unsigned font_face::glyph_count() const noexcept
{
    return glyph_count_;
}

double font_face::em_scale(double size) const
{
    advance(0, size);
    return size / units_;
}

font_face::font_face(std::vector<std::uint8_t> data, unsigned face_index) : data_(std::move(data))
{
    if (data_.size() > 128 * 1024 * 1024)
    {
        throw std::length_error("字体超过 128 MiB");
    }
    read_directory(face_index);
    read_metrics();
    choose_cmap();
}

font_face font_face::load(const std::filesystem::path& file, unsigned face_index)
{
    std::ifstream input(file, std::ios::binary | std::ios::ate);
    const auto length = input.tellg();
    if (!input || length < 0 || length > 128 * 1024 * 1024)
    {
        throw std::invalid_argument("字体文件无法读取或超过 128 MiB");
    }
    std::vector<std::uint8_t> data(static_cast<std::size_t>(length));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(data.data()), length))
    {
        throw std::invalid_argument("字体文件读取不完整");
    }
    return font_face(std::move(data), face_index);
}

void font_face::read_directory(unsigned face_index)
{
    const font_reader reader(data_);
    std::size_t base = 0;
    if (reader.u32(0) == tag("ttcf"))
    {
        const auto count = reader.u32(8);
        if (!count || count > 4096 || face_index >= count)
        {
            throw std::invalid_argument("TTC 字体索引越界");
        }
        reader.slice(12, std::size_t(count) * 4);
        base = reader.u32(12 + std::size_t(face_index) * 4);
    }
    else if (face_index != 0)
    {
        throw std::invalid_argument("单字体文件的字体索引必须为零");
    }
    const auto signature = reader.u32(base);
    if (signature != 0x00010000 && signature != tag("true"))
    {
        throw std::invalid_argument("当前字体解析器要求 TrueType glyf 轮廓");
    }
    const auto count = reader.u16(base + 4);
    if (!count || count > 4096)
    {
        throw std::invalid_argument("字体目录表数量非法");
    }
    reader.slice(base + 12, std::size_t(count) * 16);
    for (unsigned index = 0; index < count; ++index)
    {
        const auto position = base + 12 + index * 16;
        const auto offset = reader.u32(position + 8), length = reader.u32(position + 12);
        reader.slice(offset, length);
        if (!tables_.emplace(reader.u32(position), table_entry{offset, length}).second)
        {
            throw std::invalid_argument("字体目录包含重复表");
        }
    }
}

void font_face::read_metrics()
{
    const auto head = table("head"), hhea = table("hhea"), maxp = table("maxp");
    if (head.u32(12) != 0x5f0f3cf5 || head.i16(50) < 0 || head.i16(50) > 1)
    {
        throw std::invalid_argument("字体 head 表标识或 loca 格式非法");
    }
    units_ = head.u16(18);
    long_locations_ = head.i16(50) == 1;
    glyph_count_ = maxp.u16(4);
    metrics_count_ = hhea.u16(34);
    ascender_ = hhea.i16(4);
    descender_ = hhea.i16(6);
    line_gap_ = hhea.i16(8);
    if (units_ < 16 || units_ > 16384 || !glyph_count_ || !metrics_count_ || metrics_count_ > glyph_count_)
    {
        throw std::invalid_argument("字体度量数量或 em 尺寸非法");
    }
    table("hmtx").slice(0, metrics_count_ * 4 + (glyph_count_ - metrics_count_) * 2);
    const auto loca = table("loca"), glyf = table("glyf");
    std::uint32_t previous = 0;
    for (unsigned index = 0; index <= glyph_count_; ++index)
    {
        const auto offset = long_locations_ ? loca.u32(index * 4) : loca.u16(index * 2) * 2u;
        if (offset < previous || offset > glyf.size())
        {
            throw std::invalid_argument("字形位置表未递增或超出 glyf 表");
        }
        previous = offset;
    }
}
}
