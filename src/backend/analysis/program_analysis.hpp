#pragma once

#include "backend/analysis/program_ir.hpp"

namespace tx
{

struct parameter_summary
{
    bool mutated = false;
    bool captured = false;
    bool returned_alias = false;
    bool returned_content = false;
    bool rebound = false;
    friend bool operator==(const parameter_summary&, const parameter_summary&) = default;
};

struct function_summary
{
    std::vector<parameter_summary> parameters;
    std::vector<std::set<analysis_id>> parameter_contents;
    call_effects effects{false, false, false, false, false, false};
    bool returns_fresh = false;
};

struct function_analysis
{
    function_ir ir;
    function_summary summary;
    std::vector<std::set<analysis_id>> points_to;
    std::vector<std::set<analysis_id>> contents;
    std::set<analysis_id> captured;
    std::set<analysis_id> returned;
    std::set<analysis_id> mutated;

    [[nodiscard]] bool confined(const variable_declaration& declaration) const;
};

class program_analysis
{
public:
    void analyze(const program& source);
    [[nodiscard]] const function_analysis* find(const function_decl& function) const;
    [[nodiscard]] std::string dump() const;

private:
    std::unordered_map<const function_decl*, function_analysis> functions_;
    std::vector<const function_decl*> order_;
    std::size_t iterations_ = 0;
    bool solve_function(function_analysis& function);
    bool solve_instruction(function_analysis& function, analysis_id id);
    void solve_call(function_analysis& function, analysis_id id, bool& changed);
    bool solve_builtin_call(function_analysis& function, analysis_id id, bool& changed);
};

[[nodiscard]] bool merge_analysis_set(std::set<analysis_id>& destination,
    const std::set<analysis_id>& source);
[[nodiscard]] std::set<analysis_id> reachable_objects(const function_analysis& function,
    const std::set<analysis_id>& roots);
void merge_call_effects(call_effects& destination, const call_effects& source);
void refine_builtin_effects(function_ir& function);

} // namespace tx
