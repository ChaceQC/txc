#include "backend/analysis/ir_builder.hpp"

#include <algorithm>

namespace tx
{

analysis_id ir_builder::logical_value(const expression& item, const binary_operation& operation)
{
    const auto left = value(*operation.left);
    const auto short_block = current_;
    const auto right_block = block();
    const auto merge = block();
    const bool conjunction = operation.operation == token_kind::and_and;
    condition(left, conjunction ? right_block : merge, conjunction ? merge : right_block);
    current_ = right_block;
    const auto right = value(*operation.right);
    const auto right_end = current_;
    edge(right_end, merge);
    current_ = merge;
    analysis_instruction phi;
    phi.operation = ir_operation::phi;
    phi.type = item.type;
    phi.source = &item;
    phi.inputs = {left, right};
    phi.incoming_blocks = {short_block, right_end};
    return append(std::move(phi));
}

analysis_id ir_builder::operation_value(const expression& item,
    const operator_binding& binding, std::vector<analysis_id> inputs)
{
    analysis_instruction instruction;
    instruction.operation = ir_operation::call;
    instruction.type = item.type;
    instruction.source = &item;
    instruction.inputs = std::move(inputs);
    instruction.target = binding.virtual_slot ? nullptr : target(binding.symbol, binding.overload_index);
    instruction.unknown_target = !instruction.target;
    instruction.effects = {};
    for (std::size_t index = 0; index < instruction.inputs.size(); ++index)
    {
        instruction.argument_parameters.push_back(index);
    }
    return append(std::move(instruction), true);
}

analysis_id ir_builder::call_value(const expression& item, const call_expression& call)
{
    if (call.is_super_view)
    {
        analysis_instruction view;
        view.operation = ir_operation::projection;
        view.type = item.type;
        view.source = &item;
        view.inputs = {read(variable("self"))};
        return append(std::move(view));
    }
    analysis_instruction instruction;
    instruction.operation = call.is_constructor ? ir_operation::allocation : ir_operation::call;
    instruction.type = item.type;
    instruction.source = &item;
    instruction.effects = call.properties.effects;
    instruction.target = !call.indirect && !call.virtual_dispatch && call.overload_index
        ? target(call.name, *call.overload_index) : nullptr;
    const bool container = call.container_type || (call.receiver &&
        (call.receiver->type.is_vector() || call.receiver->type.is_typed_container() ||
         call.receiver->type.is_iterator() || call.receiver->type.is_option() ||
         call.receiver->type.is_result()));
    instruction.unknown_target = call.indirect || call.virtual_dispatch ||
        (!call.is_constructor && !instruction.target && !container &&
         call.name != "len" && call.name != "is_none" && call.name != "to_float" && call.name != "move");
    lower_arguments(instruction, call);
    if (call.is_constructor)
    {
        instruction.effects = {true, true, false, false, false, false};
        if (!call.constructor_init_symbol.empty())
        {
            // init 能保存构造实参或调用外部代码；未映射 self 的构造调用保守处理。
            instruction.effects = {};
            instruction.unknown_target = true;
        }
    }
    if (call.name == "move" && !call.receiver && !instruction.inputs.empty())
    {
        instruction.operation = ir_operation::projection;
        instruction.effects = {false, false, false, false, false, false};
        const auto result = append(std::move(instruction));
        if (item.ownership_id != 0)
        {
            analysis_instruction empty;
            empty.operation = ir_operation::constant;
            empty.type = item.type;
            const auto& name = std::get<name_reference>(call.arguments.front().value->data);
            bind(variable(name.name), append(std::move(empty)));
        }
        return result;
    }
    return append(std::move(instruction), true);
}

void ir_builder::lower_arguments(analysis_instruction& instruction, const call_expression& call)
{
    std::size_t offset = instruction.target && !instruction.target->owner_class.empty() ? 1 : 0;
    if (call.receiver)
    {
        instruction.inputs.push_back(value(*call.receiver));
        instruction.argument_parameters.push_back(0);
        offset = 1;
    }
    if (call.indirect)
    {
        instruction.inputs.push_back(read(variable(call.name)));
        instruction.argument_parameters.push_back(no_analysis_id);
    }
    std::size_t positional = 0;
    for (const auto& argument : call.arguments)
    {
        auto parameter = no_analysis_id;
        if (argument.kind == argument_kind::positional)
        {
            parameter = positional++ + offset;
        }
        else if (argument.kind == argument_kind::keyword && instruction.target)
        {
            const auto& parameters = instruction.target->parameters;
            const auto found = std::find_if(parameters.begin(), parameters.end(), [&](const tx::parameter& input)
            {
                return input.name == argument.name;
            });
            if (found != parameters.end())
            {
                parameter = static_cast<std::size_t>(found - parameters.begin()) + offset;
            }
        }
        instruction.inputs.push_back(value(*argument.value, true));
        instruction.argument_parameters.push_back(parameter);
    }
    if (instruction.target)
    {
        for (std::size_t index = 0; index < instruction.target->parameters.size(); ++index)
        {
            const auto& parameter = instruction.target->parameters[index];
            if (parameter.default_value && std::find(instruction.argument_parameters.begin(),
                instruction.argument_parameters.end(), index + offset) == instruction.argument_parameters.end())
            {
                instruction.inputs.push_back(value(*parameter.default_value));
                instruction.argument_parameters.push_back(index + offset);
            }
        }
    }
}

analysis_id ir_builder::value(const expression& item, bool transfer)
{
    if (const auto* name = std::get_if<name_reference>(&item.data); name && !name->function_value)
    {
        const auto id = read(variable(name->name), &item);
        result_.instructions[id].transfer_use = transfer;
        return id;
    }
    if (const auto* call = std::get_if<call_expression>(&item.data))
    {
        return call_value(item, *call);
    }
    analysis_instruction instruction;
    instruction.source = &item;
    instruction.type = item.type;
    bool may_fail = false;
    if (const auto* binary = std::get_if<binary_operation>(&item.data))
    {
        if (!binary->binding && (binary->operation == token_kind::and_and ||
            binary->operation == token_kind::or_or))
        {
            return logical_value(item, *binary);
        }
        instruction.inputs = {value(*binary->left), value(*binary->right)};
        if (binary->binding)
        {
            return operation_value(item, *binary->binding, std::move(instruction.inputs));
        }
        may_fail = true;
    }
    else if (const auto* unary = std::get_if<unary_operation>(&item.data))
    {
        instruction.inputs = {value(*unary->operand)};
        if (unary->binding)
        {
            return operation_value(item, *unary->binding, std::move(instruction.inputs));
        }
        may_fail = true;
    }
    else if (const auto* member = std::get_if<member_expression>(&item.data))
    {
        instruction.operation = ir_operation::projection;
        instruction.inputs = {value(*member->object)};
        may_fail = true;
    }
    else if (const auto* index = std::get_if<index_expression>(&item.data))
    {
        instruction.operation = ir_operation::projection;
        instruction.inputs = {value(*index->object), value(*index->index)};
        may_fail = true;
    }
    else if (const auto* cast = std::get_if<cast_expression>(&item.data))
    {
        instruction.inputs = {value(*cast->value)};
        instruction.operation = analysis_reference_type(item.type)
            ? ir_operation::projection : ir_operation::compute;
        may_fail = true;
    }
    else if (const auto* array = std::get_if<array_literal>(&item.data))
    {
        instruction.operation = ir_operation::allocation;
        for (const auto& element : array->elements)
        {
            instruction.inputs.push_back(value(*element));
        }
        may_fail = true;
    }
    else if (const auto* dictionary = std::get_if<dictionary_literal>(&item.data))
    {
        instruction.operation = ir_operation::allocation;
        for (const auto& entry : dictionary->entries)
        {
            instruction.inputs.push_back(value(*entry.key));
            instruction.inputs.push_back(value(*entry.value));
        }
        may_fail = true;
    }
    else if (const auto* update = std::get_if<update_expression>(&item.data))
    {
        instruction.inputs = {value(*update->target)};
        const auto result = append(std::move(instruction), true);
        if (const auto* name = std::get_if<name_reference>(&update->target->data))
        {
            bind(variable(name->name), result);
        }
        else
        {
            analysis_instruction store;
            store.operation = ir_operation::store;
            store.inputs = {result_.instructions[result].inputs.front(), result};
            append(std::move(store), true);
        }
        return result;
    }
    else
    {
        instruction.operation = analysis_reference_type(item.type) ? ir_operation::allocation : ir_operation::constant;
        may_fail = analysis_reference_type(item.type);
    }
    if (instruction.operation == ir_operation::allocation)
    {
        instruction.effects = {true, item.type != value_type::str_type, false, false, false, false};
    }
    else if (instruction.operation == ir_operation::compute && analysis_reference_type(item.type))
    {
        instruction.effects.allocates = true;
        instruction.effects.registers_gc_node = item.type != value_type::str_type;
    }
    return append(std::move(instruction), may_fail);
}

} // namespace tx
