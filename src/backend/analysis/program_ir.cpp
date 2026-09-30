#include "backend/analysis/ir_builder.hpp"

#include <stdexcept>

namespace tx
{

bool analysis_reference_type(const value_type& type)
{
    return type != value_type::int_type && type != value_type::float_type &&
        type != value_type::bool_type && type != value_type::void_type &&
        type != value_type::unknown_type;
}

function_lookup analysis_functions(const program& source)
{
    function_lookup result;
    for (const auto& function : source.functions)
    {
        result[function.name].push_back(&function);
    }
    const auto methods = [&](const auto& definitions)
    {
        for (const auto& definition : definitions)
        {
            for (const auto& method : definition.methods)
            {
                result[class_method_symbol(definition.name, method.name)].push_back(&method);
            }
        }
    };
    methods(source.structs);
    methods(source.classes);
    return result;
}

ir_builder::ir_builder(const function_decl& function, const function_lookup& functions, const program& source)
    : functions_(functions), source_(source)
{
    result_.function = &function;
    result_.blocks.resize(3);
    scopes_.emplace_back();
    std::size_t index = 0;
    const auto parameter = [&](std::string name, const value_type& type)
    {
        const auto local = declare(std::move(name), type, nullptr, index++);
        analysis_instruction input;
        input.operation = ir_operation::parameter;
        input.type = type;
        input.variable = local;
        append(std::move(input));
    };
    if (!function.owner_class.empty())
    {
        parameter("self", value_type(function.owner_class));
    }
    for (const auto& input : function.parameters)
    {
        parameter(input.name, parameter_is_nullable(input) ? value_type::any_type : input.type);
    }
}

function_ir ir_builder::build()
{
    statements(result_.function->body, false);
    if (current_ != no_analysis_id)
    {
        cleanup_scopes(0, true);
        edge(current_, result_.exit);
    }
    finalize_parameter_cleanups();
    return std::move(result_);
}

function_ir lower_function(const function_decl& function, const function_lookup& functions, const program& source)
{
    return ir_builder(function, functions, source).build();
}

analysis_id ir_builder::block()
{
    result_.blocks.emplace_back();
    return result_.blocks.size() - 1;
}

void ir_builder::edge(analysis_id from, analysis_id to, bool exceptional)
{
    if (from == no_analysis_id)
    {
        return;
    }
    result_.blocks[from].successors.push_back({to, exceptional});
    result_.blocks[to].predecessors.push_back(from);
}

analysis_id ir_builder::append(analysis_instruction instruction, bool may_fail)
{
    const auto id = result_.instructions.size();
    result_.instructions.push_back(std::move(instruction));
    result_.blocks[current_].instructions.push_back(id);
    if (may_fail)
    {
        // 失败边在后续 bind 前分出，处理器只能看见已经提交的局部版本。
        const auto next = block();
        const auto unwind = block();
        edge(current_, next);
        edge(current_, unwind, true);
        current_ = unwind;
        cleanup_scopes(exception_depths_.empty() ? 0 : exception_depths_.back(), false);
        edge(current_, exception_targets_.empty() ? result_.error_exit : exception_targets_.back(), true);
        current_ = next;
    }
    return id;
}

analysis_id ir_builder::declare(std::string name, const value_type& type,
    const variable_declaration* declaration, analysis_id parameter)
{
    const auto id = result_.variables.size();
    scopes_.back().emplace(name, id);
    result_.variables.push_back({std::move(name), type, parameter, declaration});
    if (parameter == no_analysis_id && analysis_reference_type(type))
    {
        // 拥有槽在入口以空指针登记。解包/迭代等操作失败时，清理边也
        // 能引用尚未提交的槽；空版本不指向任何对象，不构成额外分配。
        analysis_instruction empty;
        empty.operation = ir_operation::constant;
        empty.type = type;
        empty.variable = id;
        const auto version = result_.instructions.size();
        result_.instructions.push_back(std::move(empty));
        auto& entry = result_.blocks[result_.entry].instructions;
        entry.insert(entry.begin(), version);
    }
    return id;
}

analysis_id ir_builder::variable(std::string_view name) const
{
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope)
    {
        const auto found = scope->find(std::string(name == "super" ? "self" : name));
        if (found != scope->end())
        {
            return found->second;
        }
    }
    throw std::logic_error("分析层找不到受检局部变量：" + std::string(name));
}

analysis_id ir_builder::bind(analysis_id local, analysis_id value)
{
    analysis_instruction instruction;
    instruction.operation = ir_operation::bind_local;
    instruction.type = result_.variables[local].type;
    instruction.variable = local;
    instruction.inputs = {value};
    return append(std::move(instruction));
}

analysis_id ir_builder::read(analysis_id local, const expression* source)
{
    if (source)
    {
        result_.local_bindings.emplace(source, local);
    }
    analysis_instruction instruction;
    instruction.operation = ir_operation::read_local;
    instruction.type = result_.variables[local].type;
    instruction.variable = local;
    instruction.source = source;
    return append(std::move(instruction));
}

const function_decl* ir_builder::target(std::string_view symbol, std::size_t overload) const
{
    const auto found = functions_.find(std::string(symbol));
    return found != functions_.end() && overload < found->second.size()
        ? found->second[overload] : nullptr;
}

} // namespace tx
