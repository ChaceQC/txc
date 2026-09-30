#include "backend/analysis/program_analysis.hpp"
#include "frontend/ast/call_properties.hpp"

#include <algorithm>

namespace tx
{
namespace
{

bool default_heap_constructor(const call_expression& call)
{
    return call.container_type && call.container_type->container_name() == "heap" &&
        scalar_container_type(value_type::vector_of(call.container_type->parameters.front())) &&
        std::none_of(call.arguments.begin(), call.arguments.end(), [](const call_argument& argument)
        {
            return argument.value->type.is_function();
        });
}

bool default_heap_origin(const function_ir& function, analysis_id id,
    std::set<analysis_id>& visited, bool& has_constructor)
{
    if (!visited.insert(id).second)
    {
        return true;
    }
    const auto& instruction = function.instructions[id];
    if (instruction.operation == ir_operation::constant && !instruction.source &&
        instruction.type.container_name() == "heap")
    {
        // 初始化失败边上的空拥有槽没有比较器，也没有对象可释放。
        return true;
    }
    const auto* call = instruction.source
        ? std::get_if<call_expression>(&instruction.source->data) : nullptr;
    if (call && default_heap_constructor(*call))
    {
        has_constructor = true;
        return true;
    }
    if (instruction.operation != ir_operation::read_local &&
        instruction.operation != ir_operation::bind_local && instruction.operation != ir_operation::phi &&
        !(instruction.operation == ir_operation::projection && call && call->name == "move"))
    {
        return false;
    }
    return !instruction.inputs.empty() && std::all_of(instruction.inputs.begin(), instruction.inputs.end(),
        [&](analysis_id input)
        {
            return default_heap_origin(function, input, visited, has_constructor);
        });
}

std::set<analysis_id> contained_objects(const function_analysis& function,
    const std::set<analysis_id>& owners)
{
    std::set<analysis_id> result;
    for (const auto owner : owners)
    {
        (void)merge_analysis_set(result, function.contents[owner]);
    }
    return reachable_objects(function, result);
}

bool element_result(std::string_view name)
{
    return name == "value" || name == "value_or" || name == "error" ||
        name == "read" || name == "get" || name == "front" || name == "back" || name == "top";
}

} // namespace

void refine_builtin_effects(function_ir& function)
{
    for (auto& instruction : function.instructions)
    {
        if (instruction.operation == ir_operation::cleanup &&
            function.variables[instruction.variable].type.container_name() == "heap" &&
            !instruction.inputs.empty())
        {
            std::set<analysis_id> visited;
            bool has_constructor = false;
            if (default_heap_origin(function, instruction.inputs.front(), visited, has_constructor))
            {
                instruction.target = nullptr;
                instruction.unknown_target = false;
                instruction.effects = {false, false, false, false, false, false};
            }
        }
        const auto* call = instruction.source
            ? std::get_if<call_expression>(&instruction.source->data) : nullptr;
        if (!call || instruction.target || instruction.unknown_target)
        {
            continue;
        }
        bool default_heap = default_heap_constructor(*call);
        if (call->receiver && call->receiver->type.container_name() == "heap")
        {
            std::set<analysis_id> visited;
            bool has_constructor = false;
            default_heap = default_heap_origin(function, instruction.inputs.front(), visited, has_constructor) &&
                has_constructor;
        }
        if (default_heap)
        {
            const auto& type = call->container_type ? *call->container_type : call->receiver->type;
            // 默认堆与标量 queue 有相同的保存/修改效果；比较器不在方法间改变。
            instruction.effects = container_call_effects(value_type::container_of("queue", type.parameters),
                call->container_type ? "new" : call->name == "top" ? "front" : call->name);
        }
    }
}

bool program_analysis::solve_builtin_call(function_analysis& function, analysis_id id, bool& changed)
{
    const auto& instruction = function.ir.instructions[id];
    const auto* call = instruction.source
        ? std::get_if<call_expression>(&instruction.source->data) : nullptr;
    if (!call || instruction.target || instruction.unknown_target ||
        (!call->container_type && !call->receiver) || !instruction.effects.allows_borrow())
    {
        return false;
    }
    merge_call_effects(function.summary.effects, instruction.effects);
    const bool reference = analysis_reference_type(instruction.type);
    const bool element = call->receiver && element_result(call->name);
    const auto first_argument = call->receiver ? 1u : 0u;
    const std::set<analysis_id> owners = call->receiver
        ? function.points_to[instruction.inputs.front()] : std::set<analysis_id>{id};
    if (reference && !element)
    {
        changed |= function.points_to[id].insert(id).second;
    }
    if (call->receiver && instruction.effects.mutates_arguments)
    {
        changed |= merge_analysis_set(function.mutated, owners);
    }
    // 保存元素只建立包含边；只有容器本身逃逸时，其元素才随之逃逸。
    for (std::size_t index = first_argument; index < instruction.inputs.size(); ++index)
    {
        const auto& points = function.points_to[instruction.inputs[index]];
        if (instruction.effects.saves_arguments)
        {
            for (const auto owner : owners)
            {
                changed |= merge_analysis_set(function.contents[owner], points);
            }
        }
        if (reference && element)
        {
            changed |= merge_analysis_set(function.points_to[id], points);
        }
    }
    if (call->receiver && reference)
    {
        const auto contents = contained_objects(function, owners);
        if (element)
        {
            changed |= merge_analysis_set(function.points_to[id], contents);
        }
        else
        {
            // snapshot 持有独立容器；live iterator 必须保留源容器的引用边。
            changed |= merge_analysis_set(function.contents[id],
                call->name == "live_iter" ? owners : contents);
        }
    }
    return true;
}

} // namespace tx
