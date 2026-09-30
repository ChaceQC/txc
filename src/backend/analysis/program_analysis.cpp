#include "backend/analysis/program_analysis.hpp"

#include <algorithm>

namespace tx
{
namespace
{

bool same_effects(const call_effects& left, const call_effects& right)
{
    return left.allocates == right.allocates && left.registers_gc_node == right.registers_gc_node &&
        left.runs_user_code == right.runs_user_code && left.releases_user_objects == right.releases_user_objects &&
        left.mutates_arguments == right.mutates_arguments && left.saves_arguments == right.saves_arguments;
}

bool update_value_aliases(function_analysis& function, analysis_id id)
{
    const auto& instruction = function.ir.instructions[id];
    auto& points = function.points_to[id];
    const bool reference = analysis_reference_type(instruction.type);
    bool changed = false;
    if (instruction.operation == ir_operation::parameter ||
        instruction.operation == ir_operation::allocation ||
        (instruction.operation == ir_operation::compute && reference))
    {
        changed |= reference && points.insert(id).second;
    }
    if (instruction.operation == ir_operation::read_local ||
        instruction.operation == ir_operation::bind_local || instruction.operation == ir_operation::phi)
    {
        if (reference)
        {
            for (const auto input : instruction.inputs)
            {
                changed |= merge_analysis_set(points, function.points_to[input]);
            }
        }
        if (instruction.operation == ir_operation::bind_local)
        {
            const auto parameter = function.ir.variables[instruction.variable].parameter_index;
            if (parameter != no_analysis_id)
            {
                function.summary.parameters[parameter].rebound = true;
            }
        }
    }
    if (instruction.operation == ir_operation::projection && reference)
    {
        changed |= merge_analysis_set(points,
            reachable_objects(function, function.points_to[instruction.inputs.front()]));
    }
    return changed;
}

void update_summary(function_analysis& function)
{
    const auto captures = reachable_objects(function, function.captured);
    const auto mutations = reachable_objects(function, function.mutated);
    std::vector<analysis_id> parameter_roots(function.ir.instructions.size(), no_analysis_id);
    for (std::size_t id = 0; id < function.ir.instructions.size(); ++id)
    {
        const auto& instruction = function.ir.instructions[id];
        if (instruction.operation == ir_operation::parameter)
        {
            parameter_roots[id] = function.ir.variables[instruction.variable].parameter_index;
        }
    }
    for (std::size_t id = 0; id < function.ir.instructions.size(); ++id)
    {
        const auto& instruction = function.ir.instructions[id];
        if (instruction.operation != ir_operation::parameter)
        {
            continue;
        }
        const auto parameter = function.ir.variables[instruction.variable].parameter_index;
        auto& summary = function.summary.parameters[parameter];
        summary.mutated |= mutations.contains(id);
        summary.captured |= captures.contains(id);
        summary.returned_alias |= function.returned.contains(id);
        for (const auto root : function.returned)
        {
            summary.returned_content |= reachable_objects(function, function.contents[root]).contains(id);
        }
        for (const auto child : reachable_objects(function, function.contents[id]))
        {
            if (parameter_roots[child] != no_analysis_id)
            {
                function.summary.parameter_contents[parameter].insert(parameter_roots[child]);
            }
        }
    }
    for (const auto id : function.returned)
    {
        function.summary.returns_fresh |= function.ir.instructions[id].operation != ir_operation::parameter;
    }
    function.captured = captures;
    // 包含返回对象的引用边也逃出本函数，但直接返回别名与内部引用必须分开。
}

} // namespace

void program_analysis::analyze(const program& source)
{
    functions_.clear();
    order_.clear();
    iterations_ = 0;
    const auto functions = analysis_functions(source);
    const auto add = [&](const function_decl& definition)
    {
        if (definition.external)
        {
            return;
        }
        function_analysis analysis;
        analysis.ir = lower_function(definition, functions, source);
        construct_ssa(analysis.ir);
        refine_builtin_effects(analysis.ir);
        analyze_liveness(analysis.ir);
        analysis.summary.parameters.resize(definition.parameters.size() +
            (definition.owner_class.empty() ? 0 : 1));
        analysis.summary.parameter_contents.resize(analysis.summary.parameters.size());
        analysis.points_to.resize(analysis.ir.instructions.size());
        analysis.contents.resize(analysis.ir.instructions.size());
        order_.push_back(&definition);
        functions_.emplace(&definition, std::move(analysis));
    };
    for (const auto& function : source.functions)
    {
        add(function);
    }
    const auto methods = [&](const auto& definitions)
    {
        for (const auto& definition : definitions)
        {
            for (const auto& method : definition.methods)
            {
                add(method);
            }
        }
    };
    methods(source.structs);
    methods(source.classes);
    // 有限根集合与布尔效果均只增不减；递归环也从空摘要开始收敛。
    bool changed = true;
    while (changed)
    {
        changed = false;
        ++iterations_;
        for (const auto* definition : order_)
        {
            changed |= solve_function(functions_.at(definition));
        }
    }
}

const function_analysis* program_analysis::find(const function_decl& function) const
{
    const auto found = functions_.find(&function);
    return found == functions_.end() ? nullptr : &found->second;
}

bool program_analysis::solve_function(function_analysis& function)
{
    const auto old_summary = function.summary;
    bool changed = false;
    for (const auto& block : function.ir.blocks)
    {
        if (!block.reachable)
        {
            continue;
        }
        for (const auto id : block.instructions)
        {
            changed |= solve_instruction(function, id);
        }
    }
    // 存入调用者提供的对象，同样属于保存；不能只追踪显式 return。
    for (std::size_t id = 0; id < function.ir.instructions.size(); ++id)
    {
        if (function.ir.instructions[id].operation == ir_operation::parameter)
        {
            changed |= merge_analysis_set(function.captured, function.contents[id]);
        }
    }
    update_summary(function);
    return changed || old_summary.parameters != function.summary.parameters ||
        old_summary.parameter_contents != function.summary.parameter_contents ||
        old_summary.returns_fresh != function.summary.returns_fresh ||
        !same_effects(old_summary.effects, function.summary.effects);
}

bool program_analysis::solve_instruction(function_analysis& function, analysis_id id)
{
    const auto& instruction = function.ir.instructions[id];
    bool changed = update_value_aliases(function, id);
    if (instruction.operation == ir_operation::call ||
        (instruction.operation == ir_operation::cleanup && (instruction.target || instruction.unknown_target)) ||
        (instruction.operation == ir_operation::allocation && instruction.unknown_target))
    {
        solve_call(function, id, changed);
        if (instruction.operation == ir_operation::cleanup)
        {
            merge_call_effects(function.summary.effects, instruction.effects);
        }
    }
    else
    {
        merge_call_effects(function.summary.effects, instruction.effects);
    }
    if (instruction.operation == ir_operation::allocation && analysis_reference_type(instruction.type) &&
        instruction.type != value_type::str_type)
    {
        for (const auto input : instruction.inputs)
        {
            changed |= merge_analysis_set(function.contents[id], function.points_to[input]);
        }
    }
    if (instruction.operation == ir_operation::store)
    {
        const auto& owners = function.points_to[instruction.inputs.front()];
        changed |= merge_analysis_set(function.mutated, owners);
        for (const auto owner : owners)
        {
            changed |= merge_analysis_set(function.contents[owner], function.points_to[instruction.inputs.back()]);
        }
    }
    if (instruction.operation == ir_operation::return_value && !instruction.inputs.empty())
    {
        changed |= merge_analysis_set(function.returned, function.points_to[instruction.inputs.front()]);
    }
    return changed;
}

} // namespace tx
