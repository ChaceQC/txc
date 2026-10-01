#include "driver/compatibility.hpp"
#include "driver/compatibility_data.hpp"
#include "common/platform.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#else
#include <openssl/evp.h>
#include <memory>
#endif

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace tx
{
namespace
{

namespace fs = std::filesystem;

std::string path_text(const fs::path& path)
{
    const auto utf8 = path.u8string();
    return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}

#ifdef _WIN32
struct algorithm_handle
{
    BCRYPT_ALG_HANDLE value = nullptr;

    algorithm_handle()
    {
        if (BCryptOpenAlgorithmProvider(&value, BCRYPT_SHA256_ALGORITHM,
                                        nullptr, 0) < 0)
        {
            throw std::runtime_error("无法初始化包指纹校验");
        }
    }

    ~algorithm_handle()
    {
        BCryptCloseAlgorithmProvider(value, 0);
    }
};

struct hash_handle
{
    BCRYPT_HASH_HANDLE value = nullptr;

    hash_handle(BCRYPT_ALG_HANDLE algorithm, std::vector<unsigned char>& object)
    {
        if (BCryptCreateHash(algorithm, &value, object.data(),
                             static_cast<ULONG>(object.size()), nullptr, 0, 0) < 0)
        {
            throw std::runtime_error("无法初始化 SHA-256 计算");
        }
    }

    ~hash_handle()
    {
        BCryptDestroyHash(value);
    }
};

std::string sha256_file(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("无法读取工具链产物：" + path_text(path));
    }
    algorithm_handle algorithm;
    ULONG object_size = 0;
    ULONG returned = 0;
    if (BCryptGetProperty(algorithm.value, BCRYPT_OBJECT_LENGTH,
                          reinterpret_cast<PUCHAR>(&object_size),
                          sizeof(object_size), &returned, 0) < 0)
    {
        throw std::runtime_error("无法取得 SHA-256 工作区长度");
    }
    std::vector<unsigned char> object(object_size);
    hash_handle hash(algorithm.value, object);
    std::array<char, 64 * 1024> buffer{};
    while (input)
    {
        input.read(buffer.data(), buffer.size());
        const auto count = input.gcount();
        if (count > 0 && BCryptHashData(hash.value,
            reinterpret_cast<PUCHAR>(buffer.data()),
            static_cast<ULONG>(count), 0) < 0)
        {
            throw std::runtime_error("无法计算 SHA-256：" + path_text(path));
        }
    }
    if (!input.eof())
    {
        throw std::runtime_error("读取工具链产物失败：" + path_text(path));
    }
    std::array<unsigned char, 32> digest{};
    if (BCryptFinishHash(hash.value, digest.data(), digest.size(), 0) < 0)
    {
        throw std::runtime_error("无法完成 SHA-256：" + path_text(path));
    }
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(digest.size() * 2);
    for (const auto byte : digest)
    {
        result.push_back(hex[byte >> 4]);
        result.push_back(hex[byte & 15]);
    }
    return result;
}

#else
std::string sha256_file(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context(
        EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!input || !context || EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1)
    {
        throw std::runtime_error("无法读取或校验工具链产物：" + path_text(path));
    }
    std::array<char, 65536> buffer{};
    while (input)
    {
        input.read(buffer.data(), buffer.size());
        if (EVP_DigestUpdate(context.get(), buffer.data(), input.gcount()) != 1)
        {
            throw std::runtime_error("无法计算工具链 SHA-256");
        }
    }
    std::array<unsigned char, 32> digest{};
    unsigned size = 0;
    if (!input.eof() || EVP_DigestFinal_ex(context.get(), digest.data(), &size) != 1 || size != 32)
    {
        throw std::runtime_error("无法完成工具链 SHA-256");
    }
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    for (const auto byte : digest)
    {
        result += hex[byte >> 4];
        result += hex[byte & 15];
    }
    return result;
}
#endif

std::string manifest_field(std::ifstream& input, std::string_view name)
{
    std::string line;
    if (!std::getline(input, line))
    {
        throw std::runtime_error("兼容清单缺少字段：" + std::string(name));
    }
    if (!line.empty() && line.back() == '\r')
    {
        line.pop_back();
    }
    const auto prefix = std::string(name) + " ";
    if (!line.starts_with(prefix) || line.size() != prefix.size() + 64 ||
        !std::all_of(line.begin() + prefix.size(), line.end(),
            [](char value)
            {
                return (value >= '0' && value <= '9') ||
                       (value >= 'a' && value <= 'f');
            }))
    {
        throw std::runtime_error("兼容清单字段无效：" + std::string(name));
    }
    return line.substr(prefix.size());
}

void require_digest(const fs::path& path, std::string_view expected,
                    std::string_view name)
{
    if (sha256_file(path) != expected)
    {
        throw std::runtime_error(std::string(name) + " 与当前工具链不匹配：" +
                                 path_text(path) + "；请重新构建并整体替换 tx/ 目录");
    }
}

} // namespace

void verify_tool_interfaces(const fs::path& tool_dir)
{
    const auto interface_dir = tool_dir / "stdlib";
    if (!fs::is_directory(interface_dir))
    {
        throw std::runtime_error("缺少标准库接口目录：" + path_text(interface_dir));
    }
    for (const auto& item : compatibility_data::interfaces)
    {
        require_digest(interface_dir / std::string(item.name), item.sha256,
                       "标准库接口");
    }
    for (const auto& entry : fs::directory_iterator(interface_dir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".txh")
        {
            const auto name = path_text(entry.path().filename());
            const auto found = std::find_if(compatibility_data::interfaces.begin(),
                compatibility_data::interfaces.end(),
                [&](const auto& item)
                {
                    return item.name == name;
                });
            if (found == compatibility_data::interfaces.end())
            {
                throw std::runtime_error("发现不属于当前工具链的标准库接口：" +
                                         path_text(entry.path()));
            }
        }
    }
}

void verify_tool_package(const fs::path& tool_dir)
{
    verify_tool_interfaces(tool_dir);
    const auto manifest_path = tool_dir / "package.compat";
    std::ifstream manifest(manifest_path, std::ios::binary);
    if (!manifest)
    {
        throw std::runtime_error("缺少工具链兼容清单：" + path_text(manifest_path));
    }
    std::string header;
    if (!std::getline(manifest, header))
    {
        throw std::runtime_error("工具链兼容清单版本无效");
    }
    if (!header.empty() && header.back() == '\r')
    {
        header.pop_back();
    }
    if (header != "tx-package-v3")
    {
        throw std::runtime_error("工具链兼容清单版本无效");
    }
    const auto abi = manifest_field(manifest, "abi");
    const auto compiler = manifest_field(manifest, "txc");
    const auto library = manifest_field(manifest, "stdlib");
    const auto lto_library = manifest_field(manifest, "stdlib_lto");
    const auto clang = manifest_field(manifest, "clang");
    const auto lld = manifest_field(manifest, "lld");
#ifdef _WIN32
    for (const auto* name : {"libgcc_s_seh-1.dll", "libstdc++-6.dll",
                             "libwinpthread-1.dll", "libstdc++-u.dll",
                             "libwinpthread-u.dll",
                             "libicuin78.dll", "libicuuc78.dll",
                             "libicudt78.dll", "msquic.dll", "libpq.dll",
                             "libssl-3-x64.dll", "libcrypto-3-x64.dll", "libintl-9.dll",
                             "libiconv-2.dll", "libwinpthread-p.dll", "vcruntime140.dll"})
    {
        require_digest(tool_dir / name, manifest_field(manifest, name), name);
    }
    std::string extra;
    if (std::getline(manifest, extra))
    {
        throw std::runtime_error("工具链兼容清单存在多余内容");
    }
#else
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(tool_dir / "lib"))
    {
        if (entry.is_regular_file())
        {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    for (const auto& path : files)
    {
        const auto name = "lib/" + path.filename().string();
        require_digest(path, manifest_field(manifest, name), name);
    }
    std::string extra;
    if (files.empty() || std::getline(manifest, extra))
    {
        throw std::runtime_error("Linux 共享库清单不完整或存在多余内容");
    }
#endif
    if (abi != compatibility_data::abi_fingerprint)
    {
        throw std::runtime_error("txc 与运行时 ABI 指纹不匹配；请重新构建并整体替换 tx/ 目录");
    }
    require_digest(tool_dir / executable_name("txc"), compiler, "txc");
    require_digest(tool_dir / "libtxstdlib.a", library, "标准库静态库");
    require_digest(tool_dir / "libtxstdlib_lto.a", lto_library, "ThinLTO 标准库");
    require_digest(tool_dir / executable_name("clang"), clang, "LLVM 编译器");
    require_digest(tool_dir / "link" / executable_name("ld.lld"), lld, "LLVM 链接器");
}

} // namespace tx
