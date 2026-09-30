#include "backend/analysis/program_analysis.hpp"

namespace tx
{
namespace
{

bool copy_parameter_edges(function_analysis& function, const analysis_instruction& instruction,
    const function_summary& callee, analysis_id parameter, const std::set<analysis_id>& owners)
{
    bool changed = false;
    for (const auto contained : callee.parameter_contents[parameter])
    {
        for (std::size_t argument = 0; argument < instruction.inputs.size(); ++argument)
        {
            if (argument >= instruction.argument_parameters.size() ||
                instruction.argument_parameters[argument] != contained)
            {
                continue;
            }
            for (const auto owner : owners)
            {
                changed |= merge_analysis_set(function.contents[owner], function.points_to[instruction.inputs[argument]]);
            }
        }
    }
    return changed;
}

} // namespace

void program_analysis::solve_call(function_analysis& function, analysis_id id, bool& changed)
{
    if (solve_builtin_call(function, id, changed))
    {
        return;
    }
    const auto& instruction = function.ir.instructions[id];
    const auto* callee = instruction.target ? find(*instruction.target) : nullptr;
    const bool unknown = instruction.unknown_target ||
        (instruction.target && !instruction.target->external && !callee) ||
        (instruction.target && instruction.target->is_async);
    const auto effects = unknown ? call_effects{} : callee ? callee->summary.effects : instruction.effects;
    merge_call_effects(function.summary.effects, effects);
    const bool reference = analysis_reference_type(instruction.type);
    if (reference && (unknown || !callee || callee->summary.returns_fresh))
    {
        changed |= function.points_to[id].insert(id).second;
    }
    for (std::size_t index = 0; index < instruction.inputs.size(); ++index)
    {
        const auto input = instruction.inputs[index];
        const auto& points = function.points_to[input];
        const auto parameter = index < instruction.argument_parameters.size()
            ? instruction.argument_parameters[index] : no_analysis_id;
        const bool mapped = callee && parameter < callee->summary.parameters.size();
        const parameter_summary* summary = mapped ? &callee->summary.parameters[parameter] : nullptr;
        const bool conservative = unknown || (callee && !mapped);
        const bool captured = conservative || (summary ? summary->captured : effects.saves_arguments);
        const bool mutated = conservative || (summary ? summary->mutated : effects.mutates_arguments);
        if (captured)
        {
            changed |= merge_analysis_set(function.captured, reachable_objects(function, points));
        }
        if (mutated)
        {
            changed |= merge_analysis_set(function.mutated, points);
        }
        if (reference && (conservative || !callee || (summary && summary->returned_alias)))
        {
            changed |= merge_analysis_set(function.points_to[id], points);
        }
        if (reference && (conservative || (summary && summary->returned_content)))
        {
            changed |= merge_analysis_set(function.contents[id], reachable_objects(function, points));
        }
        if (!callee && effects.saves_arguments && !instruction.inputs.empty())
        {
            // 外部容器写入可能把其他实参存进第一个输入的对象。
            for (const auto owner : function.points_to[instruction.inputs.front()])
            {
                changed |= merge_analysis_set(function.contents[owner], points);
            }
        }
        if (mapped)
        {
            changed |= copy_parameter_edges(function, instruction, callee->summary, parameter, points);
        }
    }
}

} // namespace tx
