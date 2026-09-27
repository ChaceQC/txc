#include "stdlib/bytes.hpp"
#include "stdlib/secret.hpp"
#include "stdlib/x509.hpp"

#include <windows.h>

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

DWORD handle_count()
{
    DWORD count = 0;
    if (!GetProcessHandleCount(GetCurrentProcess(), &count))
    {
        throw std::runtime_error("GetProcessHandleCount failed");
    }
    return count;
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
