#include "stdlib/file_extra.hpp"
#include "stdlib/crypto_file_io.hpp"
#include "stdlib/encoding.hpp"
#include "stdlib/error.hpp"
#include "stdlib/filesystem_extended.hpp"

namespace tx_generated
{
namespace
{

void atomic_write(std::string_view path, std::span<const std::uint8_t> data)
{
    const auto destination = detail::checked_path(std::string(path));
    if (destination.filename().empty())
    {
        throw runtime_failure({tx::error_kind::io, "invalid_path", "目标文件路径缺少文件名"});
    }
    crypto::file_output output(destination);
    output.write(data);
    output.commit(destination);
}

} // namespace

void file_atomic_write_bytes(std::string_view path, const byte_value& data)
{
    atomic_write(path, *data);
}

void file_atomic_write_text(std::string_view path, std::string_view text, std::string_view encoding)
{
    std::string data;
    try
    {
        data = detail::encode_text(text, detail::parse_encoding(encoding));
    }
    catch (const std::runtime_error& error)
    {
        throw runtime_failure({tx::error_kind::io, "invalid_encoding", error.what()});
    }
    atomic_write(path, {reinterpret_cast<const std::uint8_t*>(data.data()), data.size()});
}

} // namespace tx_generated
