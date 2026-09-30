#include "backend/analysis/program_ir.hpp"

#include <algorithm>
#include <functional>
#include <stdexcept>

namespace tx
{
namespace
{

std::vector<analysis_id> reverse_postorder(function_ir& function)
{
    std::vector<analysis_id> order;
    const auto visit = [&](const auto& self, analysis_id block) -> void
    {
        if (function.blocks[block].reachable)
        {
            return;
        }
        function.blocks[block].reachable = true;
        for (const auto& edge : function.blocks[block].successors)
        {
            self(self, edge.block);
        }
        order.push_back(block);
    };
    visit(visit, function.entry);
    std::reverse(order.begin(), order.end());
    return order;
}

void dominators(function_ir& function, const std::vector<analysis_id>& order)
{
    std::vector<analysis_id> ranks(function.blocks.size(), no_analysis_id);
    for (std::size_t index = 0; index < order.size(); ++index)
    {
        ranks[order[index]] = index;
    }
    function.blocks[function.entry].immediate_dominator = function.entry;
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (const auto id : order)
        {
            if (id == function.entry)
            {
                continue;
            }
            auto candidate = no_analysis_id;
            for (auto predecessor : function.blocks[id].predecessors)
            {
                if (function.blocks[predecessor].immediate_dominator == no_analysis_id)
                {
                    continue;
                }
                if (candidate == no_analysis_id)
                {
                    candidate = predecessor;
                }
                else
                {
                    auto other = candidate;
                    while (predecessor != other)
                    {
                        if (ranks[predecessor] > ranks[other])
                        {
                            predecessor = function.blocks[predecessor].immediate_dominator;
                        }
                        else
                        {
                            other = function.blocks[other].immediate_dominator;
                        }
                    }
                    candidate = other;
                }
            }
            if (function.blocks[id].immediate_dominator != candidate)
            {
                function.blocks[id].immediate_dominator = candidate;
                changed = true;
            }
        }
    }
    for (const auto id : order)
    {
        if (function.blocks[id].predecessors.size() < 2)
        {
            continue;
        }
        for (auto runner : function.blocks[id].predecessors)
        {
            if (!function.blocks[runner].reachable)
            {
                continue;
            }
            while (runner != function.blocks[id].immediate_dominator)
            {
                function.blocks[runner].dominance_frontier.insert(id);
                runner = function.blocks[runner].immediate_dominator;
            }
        }
    }
}

using variable_sets = std::vector<std::set<analysis_id>>;

variable_sets live_inputs(const function_ir& function, bool include_cleanup)
{
    variable_sets inputs(function.blocks.size());
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (std::size_t index = function.blocks.size(); index-- > 0;)
        {
            const auto& block = function.blocks[index];
            if (!block.reachable)
            {
                continue;
            }
            std::set<analysis_id> live;
            for (const auto& edge : block.successors)
            {
                live.insert(inputs[edge.block].begin(), inputs[edge.block].end());
            }
            for (auto at = block.instructions.rbegin(); at != block.instructions.rend(); ++at)
            {
                const auto& instruction = function.instructions[*at];
                if (instruction.operation == ir_operation::read_local ||
                    (include_cleanup && instruction.operation == ir_operation::cleanup))
                {
                    live.insert(instruction.variable);
                }
                else if (instruction.variable != no_analysis_id && instruction.operation != ir_operation::phi &&
                    instruction.operation != ir_operation::cleanup)
                {
                    live.erase(instruction.variable);
                }
            }
            if (live != inputs[index])
            {
                inputs[index] = std::move(live);
                changed = true;
            }
        }
    }
    return inputs;
}

void place_phis(function_ir& function, const variable_sets& live)
{
    variable_sets definitions(function.variables.size());
    for (std::size_t block = 0; block < function.blocks.size(); ++block)
    {
        if (!function.blocks[block].reachable)
        {
            continue;
        }
        for (const auto id : function.blocks[block].instructions)
        {
            const auto& instruction = function.instructions[id];
            if (instruction.variable != no_analysis_id && instruction.operation != ir_operation::read_local &&
                instruction.operation != ir_operation::cleanup)
            {
                definitions[instruction.variable].insert(block);
            }
        }
    }
    for (std::size_t variable = 0; variable < definitions.size(); ++variable)
    {
        std::vector<analysis_id> pending(definitions[variable].begin(), definitions[variable].end());
        std::set<analysis_id> placed;
        while (!pending.empty())
        {
            const auto definition = pending.back();
            pending.pop_back();
            for (const auto frontier : function.blocks[definition].dominance_frontier)
            {
                // 无后续读取的作用域局部无需 phi，避免合并不存在的局部版本。
                if (!live[frontier].contains(variable) || !placed.insert(frontier).second)
                {
                    continue;
                }
                analysis_instruction phi;
                phi.operation = ir_operation::phi;
                phi.type = function.variables[variable].type;
                phi.variable = variable;
                const auto id = function.instructions.size();
                function.instructions.push_back(std::move(phi));
                auto& instructions = function.blocks[frontier].instructions;
                instructions.insert(instructions.begin(), id);
                if (!definitions[variable].contains(frontier))
                {
                    pending.push_back(frontier);
                }
            }
        }
    }
}

void rename_locals(function_ir& function)
{
    std::vector<std::vector<analysis_id>> children(function.blocks.size());
    for (std::size_t block = 0; block < function.blocks.size(); ++block)
    {
        const auto parent = function.blocks[block].immediate_dominator;
        if (parent != no_analysis_id && parent != block)
        {
            children[parent].push_back(block);
        }
    }
    std::vector<std::vector<analysis_id>> versions(function.variables.size());
    const auto current = [&](analysis_id variable)
    {
        if (versions[variable].empty())
        {
            throw std::logic_error("SSA 读取了未定义的局部版本：" + function.variables[variable].name);
        }
        return versions[variable].back();
    };
    const auto rename = [&](const auto& self, analysis_id block) -> void
    {
        std::vector<analysis_id> defined;
        for (const auto id : function.blocks[block].instructions)
        {
            auto& instruction = function.instructions[id];
            if (instruction.variable == no_analysis_id)
            {
                continue;
            }
            if (instruction.operation == ir_operation::read_local || instruction.operation == ir_operation::cleanup)
            {
                instruction.inputs = {current(instruction.variable)};
            }
            else
            {
                versions[instruction.variable].push_back(id);
                defined.push_back(instruction.variable);
            }
        }
        for (const auto& edge : function.blocks[block].successors)
        {
            for (const auto id : function.blocks[edge.block].instructions)
            {
                auto& phi = function.instructions[id];
                if (phi.operation == ir_operation::phi && phi.variable != no_analysis_id)
                {
                    phi.inputs.push_back(current(phi.variable));
                    phi.incoming_blocks.push_back(block);
                }
            }
        }
        for (const auto child : children[block])
        {
            self(self, child);
        }
        for (auto local = defined.rbegin(); local != defined.rend(); ++local)
        {
            versions[*local].pop_back();
        }
    };
    rename(rename, function.entry);
}

} // namespace

void construct_ssa(function_ir& function)
{
    const auto order = reverse_postorder(function);
    dominators(function, order);
    place_phis(function, live_inputs(function, true));
    rename_locals(function);
    verify_program_ir(function);
}

void analyze_liveness(function_ir& function)
{
    const auto inputs = live_inputs(function, false);
    for (const auto& block : function.blocks)
    {
        if (!block.reachable)
        {
            continue;
        }
        std::set<analysis_id> live;
        for (const auto& edge : block.successors)
        {
            live.insert(inputs[edge.block].begin(), inputs[edge.block].end());
        }
        for (auto at = block.instructions.rbegin(); at != block.instructions.rend(); ++at)
        {
            const auto& instruction = function.instructions[*at];
            if (instruction.operation == ir_operation::read_local)
            {
                if (instruction.transfer_use && instruction.source &&
                    analysis_reference_type(instruction.type) && !live.contains(instruction.variable))
                {
                    function.move_candidates.insert(instruction.source);
                }
                live.insert(instruction.variable);
            }
            else if (instruction.variable != no_analysis_id && instruction.operation != ir_operation::phi &&
                instruction.operation != ir_operation::cleanup)
            {
                live.erase(instruction.variable);
            }
        }
    }
}

} // namespace tx
