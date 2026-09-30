#pragma once

#include "frontend/ast/ast.hpp"

#include <limits>
#include <set>
#include <unordered_set>

namespace tx
{

using analysis_id = std::size_t;
inline constexpr analysis_id no_analysis_id = std::numeric_limits<analysis_id>::max();

enum class ir_operation
{
    parameter, constant, compute, read_local, bind_local, phi,
    allocation, projection, store, call, return_value, branch, cleanup
};

struct analysis_instruction
{
    ir_operation operation = ir_operation::compute;
    value_type type = value_type::void_type;
    std::vector<analysis_id> inputs;
    // phi 的输入与 incoming_blocks 一一对应，不能把合流误作顺序执行。
    std::vector<analysis_id> incoming_blocks;
    analysis_id variable = no_analysis_id;
    const expression* source = nullptr;
    const function_decl* target = nullptr;
    std::vector<analysis_id> argument_parameters;
    call_effects effects{false, false, false, false, false, false};
    bool unknown_target = false;
    bool transfer_use = false;
};

struct control_edge
{
    analysis_id block;
    bool exceptional = false;
};

struct analysis_block
{
    std::vector<analysis_id> instructions;
    std::vector<control_edge> successors;
    std::vector<analysis_id> predecessors;
    analysis_id immediate_dominator = no_analysis_id;
    std::set<analysis_id> dominance_frontier;
    bool reachable = false;
};

struct analysis_variable
{
    std::string name;
    value_type type;
    analysis_id parameter_index = no_analysis_id;
    const variable_declaration* declaration = nullptr;
};

struct function_ir
{
    const function_decl* function = nullptr;
    std::vector<analysis_variable> variables;
    std::vector<analysis_instruction> instructions;
    std::vector<analysis_block> blocks;
    analysis_id entry = 0;
    analysis_id exit = 1;
    analysis_id error_exit = 2;
    std::unordered_set<const expression*> move_candidates;
    // 源码引用保留词法绑定身份，供局部表示检查区分遮蔽与真正重绑定。
    std::unordered_map<const expression*, analysis_id> local_bindings;
    std::unordered_map<const unpack_assignment*, std::vector<analysis_id>> unpack_bindings;
};

using function_lookup = std::unordered_map<std::string, std::vector<const function_decl*>>;

[[nodiscard]] bool analysis_reference_type(const value_type& type);
[[nodiscard]] function_lookup analysis_functions(const program& source);
[[nodiscard]] function_ir lower_function(const function_decl& function,
    const function_lookup& functions, const program& source);
void construct_ssa(function_ir& function);
void analyze_liveness(function_ir& function);
void verify_program_ir(const function_ir& function);

} // namespace tx
