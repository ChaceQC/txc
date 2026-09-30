#include "backend/analysis/ir_builder.hpp"

namespace tx
{

void ir_builder::statements(const std::vector<stmt_ptr>& body, bool scoped)
{
    if (scoped)
    {
        scopes_.emplace_back();
    }
    for (const auto& item : body)
    {
        if (current_ == no_analysis_id)
        {
            break;
        }
        statement_value(*item);
    }
    if (scoped)
    {
        if (current_ != no_analysis_id)
        {
            cleanup_scopes(scopes_.size() - 1, true);
        }
        scopes_.pop_back();
    }
}

void ir_builder::assignment_value(const variable_assignment& assignment)
{
    const auto* name = std::get_if<name_reference>(&assignment.target->data);
    analysis_id owner = no_analysis_id;
    if (!name)
    {
        if (const auto* member = std::get_if<member_expression>(&assignment.target->data))
        {
            owner = value(*member->object);
        }
        else if (const auto* index = std::get_if<index_expression>(&assignment.target->data))
        {
            owner = value(*index->object);
            value(*index->index);
        }
    }
    auto assigned = value(*assignment.value, name && assignment.operation == token_kind::equal);
    if (assignment.binding)
    {
        assigned = operation_value(*assignment.target, *assignment.binding,
            {value(*assignment.target), assigned});
    }
    else if (assignment.operation != token_kind::equal)
    {
        analysis_instruction computation;
        computation.type = assignment.target->type;
        computation.inputs = {value(*assignment.target), assigned};
        assigned = append(std::move(computation), true);
    }
    if (name)
    {
        result_.local_bindings.emplace(assignment.target.get(), variable(name->name));
        bind(variable(name->name), assigned);
    }
    else
    {
        analysis_instruction store;
        store.operation = ir_operation::store;
        store.inputs = {owner, assigned};
        store.effects.mutates_arguments = true;
        append(std::move(store), true);
    }
}

void ir_builder::statement_value(const statement& item)
{
    if (const auto* declaration = std::get_if<variable_declaration>(&item.data))
    {
        declaration_value(*declaration);
    }
    else if (const auto* assignment = std::get_if<variable_assignment>(&item.data))
    {
        assignment_value(*assignment);
    }
    else if (const auto* unpack = std::get_if<unpack_assignment>(&item.data))
    {
        unpack_value(*unpack);
    }
    else if (const auto* branch = std::get_if<if_statement>(&item.data))
    {
        branch_value(*branch);
    }
    else if (const auto* loop = std::get_if<while_statement>(&item.data))
    {
        while_value(*loop);
    }
    else if (const auto* loop = std::get_if<for_loop>(&item.data))
    {
        range_value(*loop);
    }
    else if (const auto* loop = std::get_if<for_each>(&item.data))
    {
        foreach_value(*loop);
    }
    else if (const auto* guarded = std::get_if<try_statement>(&item.data))
    {
        try_value(*guarded);
    }
    else if (const auto* returned = std::get_if<return_statement>(&item.data))
    {
        analysis_instruction result;
        result.operation = ir_operation::return_value;
        if (returned->value)
        {
            result.inputs = {value(*returned->value, true)};
        }
        append(std::move(result));
        cleanup_scopes(0, true);
        edge(current_, result_.exit);
        current_ = no_analysis_id;
    }
    else
    {
        value(*std::get<expression_statement>(item.data).value);
    }
}

void ir_builder::declaration_value(const variable_declaration& declaration)
{
    analysis_id initial = no_analysis_id;
    if (declaration.array_length)
    {
        analysis_instruction allocation;
        allocation.operation = ir_operation::allocation;
        allocation.type = value_type::array_type;
        allocation.inputs.push_back(value(*declaration.array_length));
        if (declaration.initializer)
        {
            allocation.inputs.push_back(value(*declaration.initializer));
        }
        allocation.effects = {true, true, false, false, false, false};
        initial = append(std::move(allocation), true);
    }
    else
    {
        initial = value(*declaration.initializer, true);
    }
    bind(declare(declaration.name, declaration.declared_type.value_or(
        result_.instructions[initial].type), &declaration), initial);
}

void ir_builder::unpack_value(const unpack_assignment& unpack)
{
    const auto input = value(*unpack.value);
    for (std::size_t index = 0; index < unpack.names.size(); ++index)
    {
        const auto local = unpack.declares[index]
            ? declare(unpack.names[index], value_type::any_type) : variable(unpack.names[index]);
        result_.unpack_bindings[&unpack].push_back(local);
        analysis_instruction projection;
        projection.operation = ir_operation::projection;
        projection.type = result_.variables[local].type;
        projection.inputs = {input};
        bind(local, append(std::move(projection), true));
    }
}

void ir_builder::condition(analysis_id value, analysis_id yes, analysis_id no)
{
    analysis_instruction branch;
    branch.operation = ir_operation::branch;
    branch.inputs = {value};
    append(std::move(branch));
    edge(current_, yes);
    edge(current_, no);
}

void ir_builder::branch_value(const if_statement& branch)
{
    const auto test = value(*branch.condition);
    const auto yes = block();
    const auto no = block();
    const auto merge = block();
    condition(test, yes, no);
    current_ = yes;
    statements(branch.then_body);
    const auto yes_end = current_;
    edge(yes_end, merge);
    current_ = no;
    statements(branch.else_body);
    const auto no_end = current_;
    edge(no_end, merge);
    current_ = yes_end == no_analysis_id && no_end == no_analysis_id ? no_analysis_id : merge;
}

void ir_builder::while_value(const while_statement& loop)
{
    const auto header = block();
    const auto body = block();
    const auto after = block();
    edge(current_, header);
    current_ = header;
    const auto test = value(*loop.condition);
    condition(test, body, after);
    current_ = body;
    statements(loop.body);
    edge(current_, header);
    current_ = after;
}

void ir_builder::range_value(const for_loop& loop)
{
    const auto first = value(*loop.first);
    const auto last = value(*loop.last);
    scopes_.emplace_back();
    const auto iterator = declare(loop.name, value_type::int_type);
    bind(iterator, first);
    const auto header = block();
    const auto body = block();
    const auto after = block();
    edge(current_, header);
    current_ = header;
    analysis_instruction comparison;
    comparison.type = value_type::bool_type;
    comparison.inputs = {read(iterator), last};
    condition(append(std::move(comparison)), body, after);
    current_ = body;
    statements(loop.body);
    if (current_ != no_analysis_id)
    {
        analysis_instruction increment;
        increment.type = value_type::int_type;
        increment.inputs = {read(iterator)};
        bind(iterator, append(std::move(increment), true));
        edge(current_, header);
    }
    scopes_.pop_back();
    current_ = after;
}

void ir_builder::foreach_value(const for_each& loop)
{
    const auto values = value(*loop.values);
    scopes_.emplace_back();
    const auto iterator_type = loop.values->type.parameters.empty()
        ? value_type::any_type : loop.values->type.parameters.front();
    const auto iterator = declare(loop.name, iterator_type);
    const auto header = block();
    const auto body = block();
    const auto after = block();
    edge(current_, header);
    current_ = header;
    analysis_instruction next;
    next.operation = ir_operation::call;
    next.inputs = {values};
    next.type = value_type::bool_type;
    next.effects = {};
    condition(append(std::move(next), true), body, after);
    current_ = body;
    analysis_instruction element;
    element.operation = ir_operation::projection;
    element.type = iterator_type;
    element.inputs = {values};
    bind(iterator, append(std::move(element), true));
    statements(loop.body);
    if (current_ != no_analysis_id)
    {
        cleanup_scopes(scopes_.size() - 1, true);
    }
    edge(current_, header);
    scopes_.pop_back();
    current_ = after;
}

void ir_builder::try_value(const try_statement& guarded)
{
    const auto dispatch = block();
    const auto after = block();
    exception_targets_.push_back(dispatch);
    exception_depths_.push_back(scopes_.size());
    statements(guarded.body);
    exception_targets_.pop_back();
    exception_depths_.pop_back();
    edge(current_, after);
    for (const auto& handler : guarded.handlers)
    {
        current_ = block();
        edge(dispatch, current_);
        scopes_.emplace_back();
        analysis_instruction exception;
        exception.operation = ir_operation::allocation;
        exception.type = handler.type;
        bind(declare(handler.name, handler.type), append(std::move(exception)));
        statements(handler.body, false);
        if (current_ != no_analysis_id)
        {
            cleanup_scopes(scopes_.size() - 1, true);
        }
        scopes_.pop_back();
        edge(current_, after);
    }
    edge(dispatch, exception_targets_.empty() ? result_.error_exit : exception_targets_.back(), true);
    current_ = after;
}

} // namespace tx
