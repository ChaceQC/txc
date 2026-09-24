#pragma once

#include "frontend/ast/ast.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tx
{

class module_loader
{
public:
    explicit module_loader(std::filesystem::path standard_library_dir);
    [[nodiscard]] program load(const std::filesystem::path& root);

private:
    enum class load_state
    {
        visiting,
        loaded
    };

    [[nodiscard]] std::string load_file(const std::filesystem::path& path,
                                        const std::optional<source_pos>& import_site,
                                        program& result);
    void load_dependencies(const program& source,
                           const std::filesystem::path& path,
                           const std::string& module_key,
                           program& result);
    void load_pair(const std::filesystem::path& header_path,
                   program& header, program& result);
    [[nodiscard]] static program parse_file(const std::filesystem::path& path);
    static void append_program(program& target, program& source);
    static void validate_pair(const program& header, const program& implementation,
                              const std::filesystem::path& source_path);
    [[nodiscard]] static std::string read_source(const std::filesystem::path& path);
    [[nodiscard]] static std::string path_text(const std::filesystem::path& path);
    [[nodiscard]] std::filesystem::path resolve_import_path(
        const std::filesystem::path& source_path,
        const import_decl& dependency) const;

    std::filesystem::path standard_library_dir_;
    std::unordered_map<std::string, load_state> states_;
    std::unordered_map<std::string, std::size_t> scope_indices_;
    std::vector<std::string> stack_;
};

} // namespace tx
