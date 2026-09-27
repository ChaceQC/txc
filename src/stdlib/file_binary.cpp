#include "stdlib/file_extra.hpp"

#include "stdlib/file_stream.hpp"

namespace tx_generated
{

byte_value file_read_bytes(std::string_view path)
{
    auto source = open_binary_stream(path, "read");
    auto result = stream_read_all_bytes(source);
    stream_close(source);
    return result;
}

void file_write_bytes(std::string_view path, const byte_value& data)
{
    auto target = open_binary_stream(path, "write");
    stream_write_bytes(target, data);
    stream_close(target);
}

} // namespace tx_generated
