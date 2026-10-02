#include "stdlib/native_gui/platform/x11_connection.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace tx::ui
{
namespace
{
std::vector<std::uint8_t> authentication(const std::string& display)
{
    const auto* explicit_path = std::getenv("XAUTHORITY");
    const auto* home_path = std::getenv("HOME");
    const auto path = explicit_path && *explicit_path ? std::filesystem::path(explicit_path) :
        home_path ? std::filesystem::path(home_path) / ".Xauthority" : std::filesystem::path{};
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input)
    {
        return {};
    }
    const auto length = input.tellg();
    if (length < 0 || length > 1024 * 1024)
    {
        throw std::runtime_error("X11 认证文件长度非法");
    }
    std::vector<std::uint8_t> data(static_cast<std::size_t>(length));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(data.data()), length))
    {
        throw std::runtime_error("无法读取 X11 认证文件");
    }
    std::size_t cursor = 0;
    const auto word = [&]() -> unsigned
    {
        if (cursor + 2 > data.size())
        {
            throw std::runtime_error("X11 认证文件截断");
        }
        const auto value = (unsigned(data[cursor]) << 8) | data[cursor + 1];
        cursor += 2;
        return value;
    };
    const auto field = [&]() -> std::string
    {
        const auto count = word();
        if (count > data.size() - cursor)
        {
            throw std::runtime_error("X11 认证字段截断");
        }
        std::string result(data.begin() + cursor, data.begin() + cursor + count);
        cursor += count;
        return result;
    };
    while (cursor < data.size())
    {
        const auto family = word();
        const auto address = field(), number = field(), name = field(), secret = field();
        (void)address;
        if ((family == 256 || family == 65535) && (number.empty() || number == display) &&
            name == "MIT-MAGIC-COOKIE-1" && secret.size() == 16)
        {
            std::vector<std::uint8_t> result(secret.begin(), secret.end());
            std::fill(data.begin(), data.end(), 0);
            return result;
        }
    }
    std::fill(data.begin(), data.end(), 0);
    return {};
}
}

void x11_connection::setup(const std::string& display)
{
    auto cookie = authentication(display);
    const std::string name = cookie.empty() ? "" : "MIT-MAGIC-COOKIE-1";
    x11_packet request;
    request.u8('l');
    request.u8(0);
    request.u16(11);
    request.u16(0);
    request.u16(name.size());
    request.u16(cookie.size());
    request.u16(0);
    request.text(name);
    request.pad();
    request.bytes(cookie);
    request.pad();
    write_all(request.data);
    std::fill(cookie.begin(), cookie.end(), 0);
    std::fill(request.data.begin(), request.data.end(), 0);
    const x11_packet response{read_all(8)};
    const x11_packet setup{read_all(response.get16(6) * 4)};
    if (response.data[0] != 1)
    {
        throw std::runtime_error("X11 连接初始化或认证失败");
    }
    base_ = setup.get32(4);
    mask_ = setup.get32(8);
    maximum_request = setup.get16(18);
    minimum_key = setup.data.at(26);
    maximum_key = setup.data.at(27);
    if (!setup.data.at(20) || setup.data.at(22) != 0)
    {
        throw std::runtime_error("X11 显示缺少屏幕或不使用 little-endian 像素");
    }
    std::size_t cursor = 32 + ((setup.get16(16) + 3) & ~3u);
    bool pixels_supported = false;
    for (unsigned index = 0; index < setup.data.at(21); ++index)
    {
        if (setup.data.at(cursor) == 24 && setup.data.at(cursor + 1) == 32 && setup.data.at(cursor + 2) == 32)
        {
            pixels_supported = true;
        }
        cursor += 8;
    }
    root = setup.get32(cursor);
    depth = setup.data.at(cursor + 38);
    if (!pixels_supported || depth != 24)
    {
        throw std::runtime_error("当前 X11 像素提交要求 24-bit TrueColor / 32-bit 存储");
    }
}
}
