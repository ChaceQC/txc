#include "backend/analysis/program_ir.hpp"

#include <algorithm>
#include <stdexcept>

namespace tx
{

void verify_program_ir(const function_ir& function)
{
    std::vector<analysis_id> owners(function.instructions.size(), no_analysis_id);
    std::vector<analysis_id> positions(function.instructions.size());
    for (std::size_t block = 0; block < function.blocks.size(); ++block)
    {
        const auto& ids = function.blocks[block].instructions;
        for (std::size_t index = 0; index < ids.size(); ++index)
        {
            if (ids[index] >= owners.size() || owners[ids[index]] != no_analysis_id)
            {
                throw std::logic_error("SSA 指令必须有唯一所属基本块");
            }
            owners[ids[index]] = block;
            positions[ids[index]] = index;
        }
    }
    const auto dominates = [&](analysis_id definition, analysis_id use)
    {
        while (use != definition && use != function.entry)
        {
            use = function.blocks[use].immediate_dominator;
        }
        return use == definition;
    };
    for (std::size_t id = 0; id < function.instructions.size(); ++id)
    {
        const auto block = owners[id];
        if (block == no_analysis_id || !function.blocks[block].reachable)
        {
            continue;
        }
        const auto& instruction = function.instructions[id];
        const bool phi = instruction.operation == ir_operation::phi;
        if (phi && instruction.inputs.size() != instruction.incoming_blocks.size())
        {
            throw std::logic_error("phi 必须为每个输入记录对应前驱");
        }
        const auto predecessors = std::count_if(function.blocks[block].predecessors.begin(),
            function.blocks[block].predecessors.end(), [&](analysis_id predecessor)
            {
                return function.blocks[predecessor].reachable;
            });
        if (phi && instruction.inputs.size() != static_cast<std::size_t>(predecessors))
        {
            throw std::logic_error("phi 输入数量与 CFG 前驱不一致");
        }
        for (std::size_t index = 0; index < instruction.inputs.size(); ++index)
        {
            const auto input = instruction.inputs[index];
            const auto use_block = phi ? instruction.incoming_blocks[index] : block;
            if (input >= owners.size() || owners[input] == no_analysis_id ||
                !dominates(owners[input], use_block) ||
                (!phi && owners[input] == block && positions[input] >= positions[id]))
            {
                throw std::logic_error("SSA 输入的定义必须支配使用位置");
            }
            if (phi && std::find(function.blocks[block].predecessors.begin(),
                function.blocks[block].predecessors.end(), use_block) == function.blocks[block].predecessors.end())
            {
                throw std::logic_error("phi 输入不能来自非前驱基本块");
            }
        }
    }
}

} // namespace tx
