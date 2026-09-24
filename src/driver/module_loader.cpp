#include "driver/module_loader.hpp"

#include "frontend/lexer/lexer.hpp"
#include "frontend/parser/parser.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace tx
{
namespace
{

bool same_parameter_types(const function_decl& left,
                          const function_decl& right)
{
    if (left.parameters.size() != right.parameters.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < left.parameters.size(); ++index)
    {
        if (left.parameters[index].type != right.parameters[index].type)
        {
            return false;
        }
    }
    return true;
}

} // namespace

module_loader::module_loader(std::filesystem::path standard_library_dir)
    : standard_library_dir_(std::move(standard_library_dir))
{
}

std::string module_loader::path_text(const std::filesystem::path& path)
{
    const auto bytes = path.generic_u8string();
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

std::filesystem::path import_path(const std::string& text)
{
    std::u8string bytes;
    bytes.reserve(text.size());
    for (unsigned char value : text)
    {
        bytes.push_back(static_cast<char8_t>(value));
    }
    return std::filesystem::path(bytes);
}

std::string module_loader::read_source(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("无法读取源码文件：" + path_text(path));
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

program module_loader::parse_file(const std::filesystem::path& path)
{
    const auto source = read_source(path);
    auto tokens = lexer(source, path_text(path)).scan();
    return parser(std::move(tokens), path.extension() == ".txh").parse_program();
}

void module_loader::append_program(program& target, program& source)
{
    target.structs.insert(target.structs.end(),
                          std::make_move_iterator(source.structs.begin()),
                          std::make_move_iterator(source.structs.end()));
    target.functions.insert(target.functions.end(),
                            std::make_move_iterator(source.functions.begin()),
                            std::make_move_iterator(source.functions.end()));
}

void module_loader::validate_pair(const program& header,
                                  const program& implementation,
                                  const std::filesystem::path& source_path)
{
    for (const auto& declaration : header.functions)
    {
        const auto found = std::find_if(
            implementation.functions.begin(), implementation.functions.end(),
            [&](const function_decl& item)
            {
                return item.name == declaration.name &&
                       same_parameter_types(item, declaration);
            });
        if (found == implementation.functions.end())
        {
            throw compile_error(declaration.position,
                                "接口重载缺少实现：" + declaration.name);
        }
        if (found->return_type != declaration.return_type)
        {
            throw compile_error(found->position, "接口签名不匹配：" + found->name);
        }
        for (std::size_t index = 0; index < found->parameters.size(); ++index)
        {
            if (found->parameters[index].name != declaration.parameters[index].name)
            {
                throw compile_error(found->parameters[index].position,
                                    "接口参数不匹配：" + found->name);
            }
        }
    }
    for (const auto& definition : implementation.functions)
    {
        const auto found = std::find_if(
            header.functions.begin(), header.functions.end(),
            [&](const function_decl& item)
            {
                return item.name == definition.name &&
                       same_parameter_types(item, definition);
            });
        if (found == header.functions.end())
        {
            throw compile_error(definition.position,
                                "实现包含接口未声明的重载：" + definition.name);
        }
    }
    if (!implementation.structs.empty())
    {
        throw compile_error(implementation.structs.front().position,
                            "配对 .tx 的结构体应声明在 .txh：" + path_text(source_path));
    }
}

void module_loader::load_dependencies(
    const program& source, const std::filesystem::path& path,
    const std::string& module_key, program& result)
{
    for (const auto& dependency : source.imports)
    {
        const auto child = resolve_import_path(path, dependency);
        const auto target = load_file(child, dependency.position, result);
        auto& bindings = result.modules[scope_indices_.at(module_key)].imports;
        if (!dependency.alias.empty())
        {
            const auto duplicate = std::find_if(
                bindings.begin(), bindings.end(),
                [&](const module_import_binding& item)
                {
                    return item.alias == dependency.alias;
                });
            if (duplicate != bindings.end())
            {
                throw compile_error(dependency.position,
                                    "重复模块别名：" + dependency.alias);
            }
        }
        bindings.push_back({dependency.alias, target, dependency.position});
    }
}

std::filesystem::path module_loader::resolve_import_path(
    const std::filesystem::path& source_path,
    const import_decl& dependency) const
{
    namespace fs = std::filesystem;
    const auto requested = import_path(dependency.path);
    const auto local = source_path.parent_path() / requested;
    if (requested.has_root_path() || requested.extension() != ".txh")
    {
        return local;
    }
    const auto normalized = requested.lexically_normal();
    if (normalized.empty() || *normalized.begin() == fs::path(".."))
    {
        return local;
    }
    std::error_code error;
    const bool local_exists = fs::exists(local, error);
    if (error)
    {
        throw compile_error(dependency.position,
                            "无法检查本地接口：" + path_text(local));
    }
    if (local_exists)
    {
        return local;
    }
    const auto standard = standard_library_dir_ / normalized;
    if (fs::exists(standard, error))
    {
        return standard;
    }
    if (error)
    {
        throw compile_error(dependency.position,
                            "无法检查标准库接口：" + path_text(standard));
    }
    throw compile_error(dependency.position,
                        "找不到导入模块：" + dependency.path +
                        "（已搜索本地目录和标准库目录）");
}

void module_loader::load_pair(const std::filesystem::path& header_path,
                              program& header, program& result)
{
    namespace fs = std::filesystem;
    auto source_path = header_path;
    source_path.replace_extension(".tx");
    if (!fs::exists(source_path))
    {
        std::error_code error;
        const auto library_dir = fs::canonical(standard_library_dir_, error);
        if (!error)
        {
            auto relative = header_path.lexically_relative(library_dir);
            if (!relative.empty() && !relative.is_absolute() &&
                *relative.begin() != fs::path(".."))
            {
                relative.replace_extension();
                const auto module_name = path_text(relative);
                for (auto& function : header.functions)
                {
                    function.external_name = module_name + "." + function.name;
                }
            }
        }
        append_program(result, header);
        return;
    }
    source_path = fs::canonical(source_path);
    const auto key = path_text(source_path);
    if (states_.contains(key))
    {
        throw compile_error({1, 1, path_text(header_path)},
                            "配对模块形成循环导入：" + key);
    }
    states_.emplace(key, load_state::visiting);
    result.file_modules[key] = path_text(header_path);
    auto implementation = parse_file(source_path);
    load_dependencies(implementation, source_path, path_text(header_path), result);
    validate_pair(header, implementation, source_path);
    result.structs.insert(result.structs.end(),
                          std::make_move_iterator(header.structs.begin()),
                          std::make_move_iterator(header.structs.end()));
    result.functions.insert(result.functions.end(),
                            std::make_move_iterator(implementation.functions.begin()),
                            std::make_move_iterator(implementation.functions.end()));
    states_[key] = load_state::loaded;
}

std::string module_loader::load_file(const std::filesystem::path& path,
                                     const std::optional<source_pos>& import_site,
                                     program& result)
{
    namespace fs = std::filesystem;
    std::error_code error;
    const auto normalized = fs::canonical(path, error);
    if (error)
    {
        if (import_site)
        {
            throw compile_error(*import_site, "找不到导入模块：" + path_text(path));
        }
        throw std::runtime_error("无法读取源码文件：" + path_text(path));
    }
    const auto key = path_text(normalized);
    if (const auto found = states_.find(key); found != states_.end())
    {
        if (found->second == load_state::visiting)
        {
            const auto first = std::find(stack_.begin(), stack_.end(), key);
            if (first == stack_.end())
            {
                throw compile_error(import_site.value_or(source_pos{1, 1, key}),
                                    "配对模块的实现文件不能直接导入");
            }
            std::string chain;
            for (auto item = first; item != stack_.end(); ++item)
            {
                chain += *item + " -> ";
            }
            chain += key;
            throw compile_error(import_site.value_or(source_pos{1, 1, key}),
                                "循环导入：" + chain);
        }
        return result.file_modules.at(key);
    }
    if (import_site && normalized.extension() == ".tx")
    {
        auto header_path = normalized;
        header_path.replace_extension(".txh");
        if (fs::exists(header_path))
        {
            throw compile_error(*import_site, "此模块有 .txh 接口，请导入接口文件");
        }
    }
    states_.emplace(key, load_state::visiting);
    stack_.push_back(key);
    scope_indices_[key] = result.modules.size();
    result.modules.push_back({key, {}});
    result.file_modules[key] = key;
    auto syntax = parse_file(normalized);
    if (import_site)
    {
        for (const auto& function : syntax.functions)
        {
            if (function.name == "main")
            {
                throw compile_error(function.position, "被导入模块不能声明 main");
            }
        }
    }
    load_dependencies(syntax, normalized, key, result);
    if (normalized.extension() == ".txh")
    {
        load_pair(normalized, syntax, result);
    }
    else
    {
        append_program(result, syntax);
    }
    states_[key] = load_state::loaded;
    stack_.pop_back();
    return result.file_modules.at(key);
}

program module_loader::load(const std::filesystem::path& root)
{
    states_.clear();
    stack_.clear();
    scope_indices_.clear();
    program result;
    result.root_module = load_file(root, std::nullopt, result);
    return result;
}

} // namespace tx
