#include "stdlib/bytes.hpp"
#include "stdlib/secret.hpp"
#include "stdlib/x509.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace
{

tx_generated::byte_value read_bytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("cannot read fixture");
    }
    const std::vector<char> data(std::istreambuf_iterator<char>{input}, {});
    return tx_generated::make_bytes({data.begin(), data.end()});
}

std::size_t handle_count()
{
#ifdef _WIN32
    DWORD count = 0;
    if (!GetProcessHandleCount(GetCurrentProcess(), &count))
    {
        throw std::runtime_error("GetProcessHandleCount failed");
    }
    return count;
#else
    // 枚举时目录自身占用一个 fd；每次以同样方式读取，比较前后数量。
    const auto entries = std::filesystem::directory_iterator("/proc/self/fd");
    return static_cast<std::size_t>(std::distance(entries,
        std::filesystem::directory_iterator{}));
#endif
}

void exercise(const tx_generated::byte_value& leaf,
              const tx_generated::bytes_vector& intermediate,
              const tx_generated::bytes_vector& roots,
              const tx_generated::byte_value& package,
              const tx_generated::secret::handle& password)
{
    const auto verified = tx_generated::x509::verify(leaf, intermediate, roots,
        "service.example", "server_auth", false);
    if (verified.status != "valid" || verified.chain.size() != 3)
    {
        throw std::runtime_error("chain verification failed");
    }
    const auto certificates = tx_generated::x509::parse_pkcs12(package, password);
    if (certificates.empty() || *certificates.front() != *leaf)
    {
        throw std::runtime_error("PKCS#12 certificate mismatch");
    }
    auto private_key = tx_generated::x509::pkcs12_private_key(package, password);
    if (tx_generated::secret::size(private_key) < 100)
    {
        throw std::runtime_error("PKCS#8 key missing");
    }
    tx_generated::secret::close(private_key);
}

void check_context_isolation(const tx_generated::byte_value& leaf,
    const tx_generated::bytes_vector& intermediate,
    const tx_generated::bytes_vector& roots)
{
    using tx_generated::x509::verify;
    // 命中同一引擎后仍重验主机名和用途，不缓存验证结论。
    if (verify(leaf, intermediate, roots, "wrong.example", "server_auth", false).status !=
            "hostname_mismatch" ||
        verify(leaf, intermediate, roots, "", "client_auth", false).status != "wrong_purpose")
    {
        throw std::runtime_error("cached verifier skipped policy checks");
    }
    tx_generated::bytes_vector empty;
    if (verify(leaf, empty, roots, "service.example", "server_auth", false).status == "valid" ||
        verify(leaf, intermediate, empty, "service.example", "server_auth", false).status == "valid")
    {
        throw std::runtime_error("cached verifier leaked intermediates or trust anchors");
    }
    if (verify(leaf, intermediate, roots, "service.example", "server_auth", false).status != "valid")
    {
        throw std::runtime_error("cached verifier failed after policy error");
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 1;
    }
    try
    {
        const std::filesystem::path directory(argv[1]);
        const auto leaf = read_bytes(directory / "leaf.der");
        const auto package = read_bytes(directory / "identity.p12");
        tx_generated::bytes_vector intermediate;
        intermediate.data().values.push_back(read_bytes(directory / "intermediate.der"));
        intermediate.data().refresh();
        tx_generated::bytes_vector roots;
        roots.data().values.push_back(read_bytes(directory / "root.der"));
        roots.data().refresh();
        const std::string text = "test-only-password";
        auto password = tx_generated::secret::from_bytes(
            tx_generated::make_bytes({text.begin(), text.end()}));
        exercise(leaf, intermediate, roots, package, password);
        check_context_isolation(leaf, intermediate, roots);
        const auto before = handle_count();
        for (int index = 0; index < 20; ++index)
        {
            exercise(leaf, intermediate, roots, package, password);
        }
        const auto after = handle_count();
        tx_generated::secret::close(password);
        if (after > before + 8)
        {
            std::cerr << "X.509 resource handles grew: " << before
                      << " -> " << after << '\n';
            return 2;
        }
        std::cout << "X509_RESOURCE_OK\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 3;
    }
}
