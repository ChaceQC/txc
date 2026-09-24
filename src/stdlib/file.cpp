#include "stdlib/encoding.hpp"
#include "stdlib/stdlib.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace tx_generated
{
namespace
{

namespace fs = std::filesystem;

std::string read_bytes(const fs::path& path, const std::string& display_path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("无法打开文件进行读取：" + display_path);
    }
    std::string bytes{std::istreambuf_iterator<char>(input),
                      std::istreambuf_iterator<char>()};
    if (input.bad())
    {
        throw std::runtime_error("读取文件失败：" + display_path);
    }
    return bytes;
}

void write_bytes(const fs::path& path, const std::string& display_path,
                 const std::string& bytes, bool append)
{
    const auto mode = std::ios::binary |
        (append ? std::ios::app : std::ios::trunc);
    std::ofstream output(path, mode);
    if (!output)
    {
        throw std::runtime_error("无法打开文件进行写入：" + display_path);
    }
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.close();
    if (!output)
    {
        throw std::runtime_error("写入文件失败：" + display_path);
    }
}

std::uintmax_t existing_file_size(const fs::path& path,
                                  const std::string& display_path)
{
    std::error_code error;
    const bool exists = fs::exists(path, error);
    if (error)
    {
        throw std::runtime_error("无法检查文件：" + display_path +
                                 "：" + error.message());
    }
    if (!exists)
    {
        return 0;
    }
    const auto size = fs::file_size(path, error);
    if (error)
    {
        throw std::runtime_error("无法获取文件大小：" + display_path +
                                 "：" + error.message());
    }
    return size;
}

std::string read_first_bytes(const fs::path& path,
                             const std::string& display_path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("无法打开文件进行读取：" + display_path);
    }
    char bytes[2]{};
    input.read(bytes, 2);
    return {bytes, static_cast<std::size_t>(input.gcount())};
}

detail::text_encoding append_encoding(const fs::path& path,
                                      const std::string& display_path,
                                      detail::text_encoding encoding,
                                      std::uintmax_t size)
{
    if (size == 0 || (encoding != detail::text_encoding::utf16 &&
                      encoding != detail::text_encoding::utf16le &&
                      encoding != detail::text_encoding::utf16be))
    {
        return encoding;
    }
    const auto bom = read_first_bytes(path, display_path);
    const bool little = bom == "\xff\xfe";
    const bool big = bom == "\xfe\xff";
    if (encoding == detail::text_encoding::utf16 && !little && !big)
    {
        throw std::runtime_error("追加 UTF-16 文件需要已有 BOM：" + display_path);
    }
    if ((encoding == detail::text_encoding::utf16le && big) ||
        (encoding == detail::text_encoding::utf16be && little))
    {
        throw std::runtime_error("UTF-16 文件的 BOM 与指定字节序不一致：" + display_path);
    }
    if (encoding == detail::text_encoding::utf16)
    {
        return little ? detail::text_encoding::utf16le
                      : detail::text_encoding::utf16be;
    }
    return encoding;
}

} // namespace

std::string tx_fn_read_text(std::string path, std::string encoding)
{
    const auto selected = detail::parse_encoding(encoding);
    const auto file = detail::path_from_utf8(path);
    try
    {
        return detail::decode_text(read_bytes(file, path), selected);
    }
    catch (const std::runtime_error& error)
    {
        throw std::runtime_error("读取文本文件 " + path + " 失败：" + error.what());
    }
}

void tx_fn_write_text(std::string path, std::string text, std::string encoding)
{
    const auto selected = detail::parse_encoding(encoding);
    const auto file = detail::path_from_utf8(path);
    const auto bytes = detail::encode_text(text, selected);
    write_bytes(file, path, bytes, false);
}

void tx_fn_append_text(std::string path, std::string text, std::string encoding)
{
    const auto selected = detail::parse_encoding(encoding);
    const auto file = detail::path_from_utf8(path);
    const auto size = existing_file_size(file, path);
    const auto actual = append_encoding(file, path, selected, size);
    const auto bytes = detail::encode_text(text, actual, size == 0);
    write_bytes(file, path, bytes, true);
}

} // namespace tx_generated
