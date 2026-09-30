#include "backend/analysis/program_analysis.hpp"

namespace tx
{

bool merge_analysis_set(std::set<analysis_id>& destination, const std::set<analysis_id>& source)
{
    const auto size = destination.size();
    destination.insert(source.begin(), source.end());
    return size != destination.size();
}

std::set<analysis_id> reachable_objects(const function_analysis& function,
    const std::set<analysis_id>& roots)
{
    auto result = roots;
    std::vector<analysis_id> pending(roots.begin(), roots.end());
    while (!pending.empty())
    {
        const auto root = pending.back();
        pending.pop_back();
        for (const auto child : function.contents[root])
        {
            if (result.insert(child).second)
            {
                pending.push_back(child);
            }
        }
    }
    return result;
}

void merge_call_effects(call_effects& destination, const call_effects& source)
{
    destination.allocates |= source.allocates;
    destination.registers_gc_node |= source.registers_gc_node;
    destination.runs_user_code |= source.runs_user_code;
    destination.releases_user_objects |= source.releases_user_objects;
    destination.mutates_arguments |= source.mutates_arguments;
    destination.saves_arguments |= source.saves_arguments;
}

bool function_analysis::confined(const variable_declaration& declaration) const
{
    const auto returned_objects = reachable_objects(*this, returned);
    for (std::size_t id = 0; id < ir.instructions.size(); ++id)
    {
        const auto& instruction = ir.instructions[id];
        if (instruction.operation != ir_operation::bind_local ||
            ir.variables[instruction.variable].declaration != &declaration)
        {
            continue;
        }
        if (points_to[id].empty())
        {
            return false;
        }
        for (const auto root : points_to[id])
        {
            // 借来的参数根不等于本函数创建的独立局部存储。
            if (captured.contains(root) || returned_objects.contains(root) ||
                ir.instructions[root].operation == ir_operation::parameter)
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

} // namespace tx
