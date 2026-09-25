#include "frontend/sema/sema.hpp"

#include <string>
#include <unordered_set>
#include <utility>

namespace tx
{
namespace
{

bool returns_on_all_paths(const std::vector<stmt_ptr>& statements)
{
    for (const auto& item : statements)
    {
        if (std::holds_alternative<return_statement>(item->data))
        {
            return true;
        }
        if (const auto* branch = std::get_if<if_statement>(&item->data);
            branch != nullptr && branch->has_else &&
            returns_on_all_paths(branch->then_body) &&
            returns_on_all_paths(branch->else_body))
        {
            return true;
        }
    }
    return false;
}

bool insert_implicit_scalar_cast(expr_ptr& value, const value_type& actual,
                                 const value_type& target)
{
    if (actual != value_type::any_type ||
        (target != value_type::int_type &&
         target != value_type::float_type &&
         target != value_type::str_type))
    {
        return false;
    }
    // 目标类型已由声明、赋值或返回值确定，复用显式 as 的检查与代码生成。
    const auto position = value->position;
    auto converted = std::make_unique<expression>(
        position, cast_expression{std::move(value), target});
    converted->type = target;
    value = std::move(converted);
    return true;
}

} // namespace

void semantic_analyzer::push_scope()
{
    scopes_.emplace_back();
}

void semantic_analyzer::pop_scope()
{
    scopes_.pop_back();
}

symbol_info* semantic_analyzer::find_symbol(const std::string& name)
{
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope)
    {
        if (auto found = scope->find(name); found != scope->end())
        {
            return &found->second;
        }
    }
    return nullptr;
}

void semantic_analyzer::declare_symbol(const std::string& name,
                                       symbol_info info, source_pos position)
{
    if (name == "self" || name == "super")
    {
        throw compile_error(position, "保留的变量名：" + name);
    }
    if (scopes_.back().contains(name))
    {
        throw compile_error(position, "重复声明变量：" + name);
    }
    scopes_.back().emplace(name, std::move(info));
}

void semantic_analyzer::require_type(const value_type& actual,
                                     const value_type& expected,
                                     source_pos position, const std::string& context)
{
    if (!is_assignable(actual, expected))
    {
        throw compile_error(position, context + "需要 " + std::string(type_name(expected)) +
                                         "，实际为 " + std::string(type_name(actual)));
    }
}

void semantic_analyzer::check_function(function_decl& function)
{
    if (function.external)
    {
        return;
    }
    scopes_.clear();
    push_scope();
    current_return_type_ = function.return_type;
    const auto found_class = classes_.find(function.owner_class);
    current_class_ = found_class == classes_.end()
        ? nullptr : found_class->second;
    if (!function.owner_class.empty())
    {
        scopes_.back().emplace("self",
            symbol_info{value_type(function.owner_class), true});
        if (current_class_ != nullptr)
        {
            if (const auto* base = first_class_base(*current_class_))
            {
                scopes_.back().emplace("super",
                    symbol_info{value_type(base->name), true});
            }
        }
    }
    for (const auto& parameter : function.parameters)
    {
        declare_symbol(parameter.name, {parameter.type, false}, parameter.position);
    }
    check_statements(function.body);
    if (current_return_type_ != value_type::void_type &&
        !returns_on_all_paths(function.body))
    {
        throw compile_error(function.position, "函数缺少完整的返回路径：" +
                                               function.source_name);
    }
    pop_scope();
    current_class_ = nullptr;
}

void semantic_analyzer::check_method(function_decl& method)
{
    check_function(method);
}

void semantic_analyzer::check_statements(std::vector<stmt_ptr>& statements)
{
    for (auto& item : statements)
    {
        check_statement(*item);
    }
}

void semantic_analyzer::check_declaration(statement& item,
                                          variable_declaration& declaration)
{
    if (declaration.array_length)
    {
        require_type(check_expression(*declaration.array_length), value_type::int_type,
                     declaration.array_length->position, "数组长度");
        if (declaration.initializer)
        {
            require_type(check_expression(*declaration.initializer),
                         value_type::array_type,
                         declaration.initializer->position, "数组初值");
        }
        declare_symbol(declaration.name, {value_type::array_type, false}, item.position);
        return;
    }
    const auto inferred = check_expression(*declaration.initializer);
    if (inferred == value_type::void_type)
    {
        throw compile_error(item.position, "不能用无返回值的调用初始化变量");
    }
    const auto selected = declaration.declared_type.value_or(inferred);
    if (declaration.declared_type)
    {
        validate_type(selected, item.position);
    }
    const auto actual = declaration.declared_type &&
        insert_implicit_scalar_cast(declaration.initializer, inferred, selected)
        ? selected : inferred;
    require_type(actual, selected, declaration.initializer->position, "变量初值");
    declare_symbol(declaration.name, {selected, false}, item.position);
}

value_type semantic_analyzer::check_lvalue(expression& target)
{
    const auto type = check_expression(target);
    const expression* root = &target;
    while (true)
    {
        if (const auto* name = std::get_if<name_reference>(&root->data))
        {
            const auto* symbol = find_symbol(name->name);
            if (symbol->read_only &&
                ((name->name != "self" && name->name != "super") ||
                 root == &target))
            {
                throw compile_error(target.position, "循环变量不可赋值：" + name->name);
            }
            return type;
        }
        if (const auto* access = std::get_if<index_expression>(&root->data))
        {
            root = access->object.get();
        }
        else if (const auto* access = std::get_if<member_expression>(&root->data))
        {
            if (access->object->type == value_type::any_type)
            {
                throw compile_error(target.position, "数组元素中的结构体字段暂不支持直接赋值");
            }
            root = access->object.get();
        }
        else
        {
            throw compile_error(target.position, "赋值左侧必须是变量、字段或数组元素");
        }
    }
}

void semantic_analyzer::check_assignment(statement& item,
                                         variable_assignment& assignment)
{
    const auto target_type = check_lvalue(*assignment.target);
    auto value = check_expression(*assignment.value);
    if (value == value_type::void_type)
    {
        throw compile_error(assignment.value->position, "不能把无返回值的调用赋入变量");
    }
    if (assignment.operation == token_kind::plus_equal)
    {
        if (structs_.contains(target_type.name) ||
            classes_.contains(target_type.name))
        {
            const auto result = bind_operator(token_kind::plus, target_type,
                value, *assignment.target, item.position, assignment.binding);
            require_type(result, target_type, item.position, "+= 结果");
            return;
        }
        if (target_type != value_type::int_type &&
            target_type != value_type::float_type &&
            target_type != value_type::str_type)
        {
            throw compile_error(item.position, "+= 只支持 int、float 或 str");
        }
        require_type(value, target_type, assignment.value->position, "+= 右侧");
    }
    else if (assignment.operation == token_kind::minus_equal)
    {
        if (structs_.contains(target_type.name) ||
            classes_.contains(target_type.name))
        {
            const auto result = bind_operator(token_kind::minus, target_type,
                value, *assignment.target, item.position, assignment.binding);
            require_type(result, target_type, item.position, "-= 结果");
            return;
        }
        if (target_type != value_type::int_type &&
            target_type != value_type::float_type)
        {
            throw compile_error(item.position, "-= 只支持 int 或 float");
        }
        require_type(value, target_type, assignment.value->position, "-= 右侧");
    }
    else if (target_type != value_type::any_type)
    {
        const auto* index = std::get_if<index_expression>(&assignment.target->data);
        const bool vector_element = index && index->object->type.is_vector();
        if (!vector_element && insert_implicit_scalar_cast(assignment.value, value, target_type))
        {
            value = target_type;
        }
        require_type(value, target_type, assignment.value->position, "赋值");
    }
}

void semantic_analyzer::check_unpack(statement& item,
                                     unpack_assignment& assignment)
{
    const auto source_type = check_expression(*assignment.value);
    if (source_type != value_type::array_type && source_type != value_type::any_type)
    {
        throw compile_error(assignment.value->position, "解包右侧需要数组");
    }
    std::unordered_set<std::string> used;
    for (const auto& name : assignment.names)
    {
        if (!used.insert(name).second)
        {
            throw compile_error(item.position, "解包变量名重复：" + name);
        }
        const auto* existing = find_symbol(name);
        if (existing && existing->read_only)
        {
            throw compile_error(item.position, "循环变量不可赋值：" + name);
        }
        assignment.declares.push_back(existing == nullptr);
        if (!existing)
        {
            declare_symbol(name, {value_type::any_type, false}, item.position);
        }
    }
}

void semantic_analyzer::check_for(statement& item, for_loop& loop)
{
    require_type(check_expression(*loop.first), value_type::int_type,
                 loop.first->position, "区间起点");
    require_type(check_expression(*loop.last), value_type::int_type,
                 loop.last->position, "区间终点");
    push_scope();
    declare_symbol(loop.name, {value_type::int_type, true}, item.position);
    check_statements(loop.body);
    pop_scope();
}

void semantic_analyzer::check_for_each(statement& item, for_each& loop)
{
    const auto values_type = check_expression(*loop.values);
    if (values_type != value_type::array_type &&
        values_type != value_type::dict_type && !values_type.is_vector())
    {
        throw compile_error(loop.values->position, "遍历对象需要数组或字典");
    }
    push_scope();
    declare_symbol(loop.name, {values_type.is_vector() ? values_type.parameters.front()
                                                     : value_type::any_type, true}, item.position);
    check_statements(loop.body);
    pop_scope();
}

void semantic_analyzer::check_if(statement& item, if_statement& branch)
{
    require_type(check_expression(*branch.condition), value_type::bool_type,
                 item.position, "if 条件");
    push_scope();
    check_statements(branch.then_body);
    pop_scope();
    if (branch.has_else)
    {
        push_scope();
        check_statements(branch.else_body);
        pop_scope();
    }
}

void semantic_analyzer::check_while(statement& item, while_statement& loop)
{
    require_type(check_expression(*loop.condition), value_type::bool_type,
                 item.position, "while 条件");
    push_scope();
    check_statements(loop.body);
    pop_scope();
}

void semantic_analyzer::check_statement(statement& item)
{
    if (auto* declaration = std::get_if<variable_declaration>(&item.data))
    {
        check_declaration(item, *declaration);
    }
    else if (auto* assignment = std::get_if<variable_assignment>(&item.data))
    {
        check_assignment(item, *assignment);
    }
    else if (auto* assignment = std::get_if<unpack_assignment>(&item.data))
    {
        check_unpack(item, *assignment);
    }
    else if (auto* loop = std::get_if<for_loop>(&item.data))
    {
        check_for(item, *loop);
    }
    else if (auto* loop = std::get_if<for_each>(&item.data))
    {
        check_for_each(item, *loop);
    }
    else if (auto* branch = std::get_if<if_statement>(&item.data))
    {
        check_if(item, *branch);
    }
    else if (auto* loop = std::get_if<while_statement>(&item.data))
    {
        check_while(item, *loop);
    }
    else if (auto* result = std::get_if<return_statement>(&item.data))
    {
        if (current_return_type_ == value_type::void_type)
        {
            if (result->value)
            {
                throw compile_error(item.position, "void 函数不能返回值");
            }
            return;
        }
        if (!result->value)
        {
            throw compile_error(item.position, "非 void 函数必须返回值");
        }
        auto actual = check_expression(*result->value);
        if (insert_implicit_scalar_cast(result->value, actual,
                                        current_return_type_))
        {
            actual = current_return_type_;
        }
        require_type(actual, current_return_type_,
                     result->value->position, "返回值");
    }
    else if (auto* expression_only = std::get_if<expression_statement>(&item.data))
    {
        (void)check_expression(*expression_only->value);
    }
}

void semantic_analyzer::analyze(program& source, bool require_main)
{
    index_classes(source);
    register_structs(source);
    register_classes(source);
    register_functions(source, require_main);
    for (auto& function : source.functions)
    {
        check_function(function);
    }
    for (auto& definition : source.classes)
    {
        for (auto& method : definition.methods)
        {
            check_method(method);
        }
    }
    for (auto& definition : source.structs)
    {
        for (auto& method : definition.methods)
        {
            check_method(method);
        }
    }
}

} // namespace tx
