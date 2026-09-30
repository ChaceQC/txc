#include "backend/analysis/program_analysis.hpp"

#include <sstream>

namespace tx
{
namespace
{

std::string json_text(std::string_view value)
{
    std::ostringstream result;
    result << '"';
    constexpr char digits[] = "0123456789abcdef";
    for (const unsigned char character : value)
    {
        if (character == '"' || character == '\\')
        {
            result << '\\' << character;
        }
        else if (character < 32)
        {
            result << "\\u00" << digits[character >> 4] << digits[character & 15];
        }
        else
        {
            result << character;
        }
    }
    result << '"';
    return result.str();
}

template<typename ids_type>
void write_ids(std::ostream& output, const ids_type& ids)
{
    output << '[';
    bool first = true;
    for (const auto id : ids)
    {
        output << (first ? "" : ",") << id;
        first = false;
    }
    output << ']';
}

std::string_view operation_name(ir_operation operation)
{
    switch (operation)
    {
    case ir_operation::parameter: return "parameter";
    case ir_operation::constant: return "constant";
    case ir_operation::compute: return "compute";
    case ir_operation::read_local: return "read_local";
    case ir_operation::bind_local: return "bind_local";
    case ir_operation::phi: return "phi";
    case ir_operation::allocation: return "allocation";
    case ir_operation::projection: return "projection";
    case ir_operation::store: return "store";
    case ir_operation::call: return "call";
    case ir_operation::return_value: return "return";
    case ir_operation::branch: return "branch";
    case ir_operation::cleanup: return "cleanup";
    }
    return "unknown";
}

void write_summary(std::ostream& output, const function_summary& summary)
{
    output << "\"parameters\":[";
    for (std::size_t index = 0; index < summary.parameters.size(); ++index)
    {
        const auto& parameter = summary.parameters[index];
        output << (index ? "," : "") << "{\"mutated\":" << parameter.mutated
            << ",\"captured\":" << parameter.captured
            << ",\"returned_alias\":" << parameter.returned_alias
            << ",\"returned_content\":" << parameter.returned_content
            << ",\"rebound\":" << parameter.rebound << ",\"contains_parameters\":";
        write_ids(output, summary.parameter_contents[index]);
        output << '}';
    }
    const auto& effects = summary.effects;
    output << "],\"effects\":{\"allocates\":" << effects.allocates
        << ",\"gc\":" << effects.registers_gc_node << ",\"user_code\":" << effects.runs_user_code
        << ",\"release_user_objects\":" << effects.releases_user_objects
        << ",\"mutates\":" << effects.mutates_arguments << ",\"captures\":" << effects.saves_arguments
        << "},\"returns_fresh\":" << summary.returns_fresh;
}

void write_instruction(std::ostream& output, const function_analysis& function, analysis_id id)
{
    const auto& instruction = function.ir.instructions[id];
    output << "{\"id\":" << id << ",\"operation\":" << json_text(operation_name(instruction.operation))
        << ",\"type\":" << json_text(instruction.type.name) << ",\"inputs\":";
    write_ids(output, instruction.inputs);
    output << ",\"incoming\":";
    write_ids(output, instruction.incoming_blocks);
    output << ",\"aliases\":";
    write_ids(output, function.points_to[id]);
    if (instruction.variable != no_analysis_id)
    {
        output << ",\"variable\":" << instruction.variable;
    }
    if (instruction.source)
    {
        output << ",\"line\":" << instruction.source->position.line
            << ",\"move_candidate\":" << function.ir.move_candidates.contains(instruction.source);
    }
    output << '}';
}

void write_blocks(std::ostream& output, const function_analysis& function)
{
    output << ",\"blocks\":[";
    for (std::size_t index = 0; index < function.ir.blocks.size(); ++index)
    {
        const auto& block = function.ir.blocks[index];
        output << (index ? "," : "") << "{\"id\":" << index << ",\"reachable\":" << block.reachable
            << ",\"dominator\":";
        if (block.immediate_dominator == no_analysis_id)
        {
            output << "null";
        }
        else
        {
            output << block.immediate_dominator;
        }
        output << ",\"successors\":[";
        for (std::size_t edge = 0; edge < block.successors.size(); ++edge)
        {
            output << (edge ? "," : "") << "{\"block\":" << block.successors[edge].block
                << ",\"exceptional\":" << block.successors[edge].exceptional << '}';
        }
        output << "],\"instructions\":[";
        for (std::size_t at = 0; at < block.instructions.size(); ++at)
        {
            output << (at ? "," : "");
            write_instruction(output, function, block.instructions[at]);
        }
        output << "]}";
    }
    output << ']';
}

void write_variables(std::ostream& output, const function_ir& function)
{
    output << ",\"variables\":[";
    for (std::size_t index = 0; index < function.variables.size(); ++index)
    {
        const auto& variable = function.variables[index];
        output << (index ? "," : "") << "{\"id\":" << index << ",\"name\":" << json_text(variable.name)
            << ",\"type\":" << json_text(variable.type.name) << ",\"parameter\":";
        if (variable.parameter_index == no_analysis_id)
        {
            output << "null";
        }
        else
        {
            output << variable.parameter_index;
        }
        output << '}';
    }
    output << ']';
}

} // namespace

std::string program_analysis::dump() const
{
    std::ostringstream output;
    output << std::boolalpha << "{\"version\":1,\"iterations\":" << iterations_ << ",\"functions\":[";
    for (std::size_t index = 0; index < order_.size(); ++index)
    {
        const auto& function = functions_.at(order_[index]);
        const auto& definition = *order_[index];
        const auto symbol = definition.owner_class.empty() ? definition.name
            : class_method_symbol(definition.owner_class, definition.name);
        output << (index ? ",\n" : "\n") << "{\"name\":" << json_text(symbol)
            << ",\"overload\":" << definition.overload_index << ',';
        write_summary(output, function.summary);
        write_variables(output, function.ir);
        output << ",\"captured_roots\":";
        write_ids(output, function.captured);
        output << ",\"returned_roots\":";
        write_ids(output, function.returned);
        write_blocks(output, function);
        output << '}';
    }
    output << "\n]}\n";
    return output.str();
}

} // namespace tx
