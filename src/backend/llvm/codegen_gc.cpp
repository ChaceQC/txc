#include "backend/llvm/codegen.hpp"
#include "frontend/ast/call_properties.hpp"

#include <algorithm>

namespace tx
{

bool llvm_code_generator::gc_neutral_binding(const operator_binding& binding)
{
    if (binding.virtual_slot)
    {
        if (library_mode_)
        {
            // 预编译库无法穷举最终程序新增的派生类，虚调用继续保守轮询。
            return false;
        }
        bool found_target = false;
        for (const auto& [name, definition] : classes_)
        {
            (void)name;
            // 抽象类与接口不能成为实际接收者；只汇总可实例化类的虚目标。
            if (definition->is_abstract || definition->is_interface)
            {
                continue;
            }
            if (*binding.virtual_slot >= definition->virtual_targets.size())
            {
                continue;
            }
            const auto& target = definition->virtual_targets[*binding.virtual_slot];
            if (target.symbol.empty())
            {
                continue;
            }
            found_target = true;
            const auto found = functions_.find(target.symbol);
            if (found == functions_.end() ||
                target.overload_index >= found->second.size() ||
                !gc_neutral_function(*found->second[target.overload_index]))
            {
                return false;
            }
        }
        return found_target;
    }
    const auto found = functions_.find(binding.symbol);
    return found != functions_.end() &&
        binding.overload_index < found->second.size() &&
        gc_neutral_function(*found->second[binding.overload_index]);
}

bool llvm_code_generator::gc_neutral_expression(const expression& item)
{
    if (gc_visiting_.empty() && emitted_bounded_calls_.contains(&item))
    {
        return true;
    }
    if (std::holds_alternative<integer_literal>(item.data) ||
        std::holds_alternative<floating_literal>(item.data) ||
        std::holds_alternative<boolean_literal>(item.data))
    {
        return true;
    }
    if (std::holds_alternative<string_literal>(item.data) ||
        std::holds_alternative<none_literal>(item.data))
    {
        return false;
    }
    if (const auto* name = std::get_if<name_reference>(&item.data))
    {
        if (name->function_value)
        {
            return false;
        }
        // 作为普通值读取局部标量 option 时会在逃逸边界生成句柄。
        return !is_parse_result_type(item.type) && (!item.type.is_option() ||
            !scalar_option_suffix(item.type.parameters.front()));
    }
    if (const auto* member = std::get_if<member_expression>(&item.data))
    {
        if (is_parse_result_type(member->object->type) &&
            std::holds_alternative<name_reference>(member->object->data) &&
            (member->field == "ok" || member->field == "value"))
        {
            return true;
        }
        return !is_value_handle(item.type) && item.type != value_type::str_type &&
            gc_neutral_expression(*member->object);
    }
    if (const auto* index = std::get_if<index_expression>(&item.data))
    {
        return !is_value_handle(item.type) && item.type != value_type::str_type &&
               gc_neutral_expression(*index->object) &&
               gc_neutral_expression(*index->index);
    }
    if (const auto* cast = std::get_if<cast_expression>(&item.data))
    {
        if (const auto* index = std::get_if<index_expression>(&cast->value->data);
            index && index->object->type == value_type::array_type &&
            std::holds_alternative<name_reference>(index->object->data) &&
            (cast->target == value_type::int_type || cast->target == value_type::float_type ||
             cast->target == value_type::bool_type))
        {
            // cast_array_element 借用原槽位直接转换标量，不创建 any 根或运行用户代码。
            return gc_neutral_expression(*index->index);
        }
        return !is_value_handle(item.type) && item.type != value_type::str_type &&
            gc_neutral_expression(*cast->value);
    }
    if (const auto* unary = std::get_if<unary_operation>(&item.data))
    {
        return gc_neutral_expression(*unary->operand) &&
               (!unary->binding || gc_neutral_binding(*unary->binding));
    }
    if (const auto* update = std::get_if<update_expression>(&item.data))
    {
        if (const auto* member = std::get_if<member_expression>(&update->target->data);
            member && is_parse_result_type(member->object->type))
        {
            return false;
        }
        if (const auto* index = std::get_if<index_expression>(&update->target->data);
            index && (index->object->type.is_typed_container() ||
                      index->object->type.is_vector()))
        {
            return false;
        }
        return gc_neutral_expression(*update->target);
    }
    if (const auto* binary = std::get_if<binary_operation>(&item.data))
    {
        if (binary->binding && native_record_target(*binary->binding) && !scalar_record_type(item.type))
        {
            return native_record_gc_neutral(*binary->left) && native_record_gc_neutral(*binary->right);
        }
        return item.type != value_type::str_type &&
               gc_neutral_expression(*binary->left) &&
               gc_neutral_expression(*binary->right) &&
               (!binary->binding || gc_neutral_binding(*binary->binding));
    }
    const auto* call = std::get_if<call_expression>(&item.data);
    if (call && native_record_target(*call) && !scalar_record_type(item.type))
    {
        return (!call->receiver || native_record_gc_neutral(*call->receiver)) &&
            std::all_of(call->arguments.begin(), call->arguments.end(), [&](const call_argument& argument)
            {
                return native_record_gc_neutral(*argument.value);
            });
    }
    if (!call || call->is_constructor)
    {
        return false;
    }
    if (gc_visiting_.empty() && !call->receiver && !call->overload_index &&
        call->name == "is_none" && call->arguments.size() == 1)
    {
        const auto* index = std::get_if<index_expression>(&call->arguments.front().value->data);
        const auto* name = index ? std::get_if<name_reference>(&index->object->data) : nullptr;
        if (name && has_local_array(find_variable(name->name, item.position)))
        {
            // 局部标量数组只读取 tag；动态长度与栈数组使用同一条生成路径。
            return gc_neutral_expression(*index->index);
        }
    }
    if (gc_visiting_.empty() && !call->receiver && !call->overload_index &&
        call->name == "len" && call->arguments.size() == 1)
    {
        const auto& argument = *call->arguments.front().value;
        const auto* member = std::get_if<member_expression>(&argument.data);
        const auto* name = member ? std::get_if<name_reference>(&member->object->data) : nullptr;
        if (name && argument.type == value_type::str_type &&
            !find_variable(name->name, argument.position).readonly_parse_error.empty())
        {
            return true;
        }
    }
    if (call->is_super_view)
    {
        return true;
    }
    if (call->receiver && call->receiver->type.is_option() &&
        scalar_option_suffix(call->receiver->type.parameters.front()) &&
        (call->name == "value" || call->name == "is_some" ||
         call->name == "is_none"))
    {
        // 名称接收者可直接借用或读取栈值；复杂接收者可能先创建句柄。
        return item.type != value_type::str_type &&
            std::holds_alternative<name_reference>(call->receiver->data);
    }
    if (call->indirect)
    {
        // 函数摘要递归分析不借用当前调用方的局部符号环境。
        if (!gc_visiting_.empty())
        {
            return false;
        }
        const auto& slot = find_variable(call->source_name, item.position);
        return slot.closure_function && gc_neutral_function(*slot.closure_function) &&
            std::all_of(call->arguments.begin(), call->arguments.end(),
                [&](const call_argument& argument)
                {
                    const auto& type = argument.value->type;
                    return (type == value_type::int_type || type == value_type::float_type ||
                            type == value_type::bool_type) &&
                        gc_neutral_expression(*argument.value);
                });
    }
    if (gc_visiting_.empty() && default_heap_receiver(*call))
    {
        return std::all_of(call->arguments.begin(), call->arguments.end(),
            [&](const call_argument& argument)
            {
                return stable_borrow_expression(*argument.value) &&
                    gc_neutral_expression(*argument.value);
            });
    }
    if (call->receiver && !gc_neutral_expression(*call->receiver))
    {
        return false;
    }
    for (std::size_t index = 0; index < call->arguments.size(); ++index)
    {
        const auto& argument = call->arguments[index];
        const auto& value = *argument.value;
        // 具名和展开绑定会物化参数；文本/none 字面量会创建句柄。
        if (argument.kind != argument_kind::positional ||
            std::holds_alternative<string_literal>(value.data) ||
            std::holds_alternative<none_literal>(value.data) ||
            !gc_neutral_expression(value))
        {
            return false;
        }
        if ((is_value_handle(value.type) || value.type == value_type::str_type) &&
            (index >= call->properties.arguments.size() ||
             call->properties.arguments[index] != argument_ownership::borrowed ||
             !stable_borrow_expression(value)))
        {
            return false;
        }
    }
    const auto* receiver_call = call->receiver
        ? std::get_if<call_expression>(&call->receiver->data) : nullptr;
    if (call->receiver && is_value_handle(call->receiver->type) &&
        ((!(receiver_call && receiver_call->is_super_view) && !stable_borrow_expression(*call->receiver)) ||
         (!classes_.contains(call->receiver->type.name) &&
          ((!call->receiver->type.is_typed_container() && !call->receiver->type.is_vector()) ||
           call->properties.receiver != argument_ownership::borrowed))))
    {
        return false;
    }
    if (call->container_type || (call->receiver &&
        (call->receiver->type.is_typed_container() ||
         call->receiver->type.is_vector())) ||
        (!call->overload_index && (call->name == "len" ||
         call->name == "is_none" || call->name == "to_float")))
    {
        return !call->properties.effects.needs_gc_safepoint();
    }
    if (!call->overload_index)
    {
        return false;
    }
    const auto found = functions_.find(call->name);
    if (found == functions_.end() ||
        *call->overload_index >= found->second.size())
    {
        return false;
    }
    const auto& target = *found->second[*call->overload_index];
    if (target.external && !call->virtual_dispatch)
    {
        return !call->properties.effects.needs_gc_safepoint();
    }
    if (std::any_of(target.parameters.begin(), target.parameters.end(),
        [](const parameter& value)
        { return value.kind != parameter_kind::ordinary; }))
    {
        return false;
    }
    if (target.is_async || target.parameters.size() != call->arguments.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < call->arguments.size(); ++index)
    {
        if (call->arguments[index].value->type != target.parameters[index].type)
        {
            // 隐式转换可物化句柄，不能只依据被调用函数体省略安全点。
            return false;
        }
    }
    if (call->virtual_dispatch)
    {
        return gc_neutral_binding({call->name, *call->overload_index,
                                   call->virtual_slot});
    }
    return gc_neutral_function(target);
}

bool llvm_code_generator::gc_neutral_native_option_declaration(
    const variable_declaration& declaration)
{
    if (declaration.array_length || !declaration.initializer)
    {
        return false;
    }
    const auto& type = declaration.declared_type
        ? *declaration.declared_type : declaration.initializer->type;
    if (!type.is_option() || !scalar_option_suffix(type.parameters.front()))
    {
        return false;
    }
    const auto* call = std::get_if<call_expression>(&declaration.initializer->data);
    if (!call)
    {
        return false;
    }
    if (call->container_type && call->container_type->is_option())
    {
        return std::all_of(call->arguments.begin(), call->arguments.end(),
            [this](const call_argument& argument)
            {
                return gc_neutral_expression(*argument.value);
            });
    }
    return call->receiver && call->receiver->type.is_iterator() &&
        call->name == "next" && gc_neutral_expression(*call->receiver);
}

bool llvm_code_generator::gc_neutral_statement(const statement& item)
{
    if (const auto* declaration =
            std::get_if<variable_declaration>(&item.data))
    {
        if (const auto* call = native_parse_initializer(*declaration))
        {
            return gc_neutral_native_parse_declaration(*call);
        }
        if (gc_neutral_native_option_declaration(*declaration))
        {
            return true;
        }
        return !declaration->array_length && declaration->initializer &&
               gc_neutral_expression(*declaration->initializer);
    }
    if (const auto* assignment =
            std::get_if<variable_assignment>(&item.data))
    {
        if (const auto* index = std::get_if<index_expression>(&assignment->target->data);
            gc_visiting_.empty() && index && !assignment->binding &&
            assignment->operation == token_kind::equal)
        {
            const auto* name = std::get_if<name_reference>(&index->object->data);
            if (name && has_local_array(find_variable(name->name, item.position)))
            {
                return gc_neutral_expression(*index->index) &&
                    (std::holds_alternative<none_literal>(assignment->value->data) ||
                     gc_neutral_expression(*assignment->value));
            }
        }
        if (const auto* member = std::get_if<member_expression>(&assignment->target->data);
            member && is_parse_result_type(member->object->type))
        {
            return false;
        }
        if (const auto* index = std::get_if<index_expression>(&assignment->target->data);
            index && (index->object->type.is_typed_container() ||
                      index->object->type.is_vector()) &&
            container_call_effects(index->object->type, "set").needs_gc_safepoint())
        {
            return false;
        }
        if (is_value_handle(assignment->target->type))
        {
            // 覆盖旧引用可能触发 deinit，进而创建需要循环回收的节点。
            return false;
        }
        return gc_neutral_expression(*assignment->target) &&
               gc_neutral_expression(*assignment->value) &&
               (!assignment->binding ||
                gc_neutral_binding(*assignment->binding));
    }
    if (const auto* unpack = std::get_if<unpack_assignment>(&item.data))
    {
        for (std::size_t index = 0; index < unpack->names.size(); ++index)
        {
            if (!unpack->declares[index] &&
                is_value_handle(find_variable(unpack->names[index],
                                              item.position).type))
            {
                return false;
            }
        }
        return gc_neutral_expression(*unpack->value);
    }
    if (const auto* branch = std::get_if<if_statement>(&item.data))
    {
        return gc_neutral_expression(*branch->condition);
    }
    if (const auto* loop = std::get_if<while_statement>(&item.data))
    {
        return gc_neutral_expression(*loop->condition);
    }
    if (const auto* loop = std::get_if<for_loop>(&item.data))
    {
        return gc_neutral_expression(*loop->first) &&
               gc_neutral_expression(*loop->last);
    }
    if (const auto* loop = std::get_if<for_each>(&item.data))
    {
        return gc_neutral_expression(*loop->values);
    }
    if (const auto* expression_only =
            std::get_if<expression_statement>(&item.data))
    {
        return !is_value_handle(expression_only->value->type) &&
               gc_neutral_expression(*expression_only->value);
    }
    return false;
}

bool llvm_code_generator::gc_neutral_body(const std::vector<stmt_ptr>& body)
{
    for (const auto& item : body)
    {
        if (const auto* declaration =
                std::get_if<variable_declaration>(&item->data))
        {
            const bool native_option =
                gc_neutral_native_option_declaration(*declaration);
            if ((!native_option && declaration->declared_type &&
                 is_value_handle(*declaration->declared_type)) ||
                (!native_option && declaration->initializer &&
                 is_value_handle(declaration->initializer->type)) ||
                !gc_neutral_statement(*item))
            {
                return false;
            }
        }
        else if (const auto* result =
                std::get_if<return_statement>(&item->data))
        {
            if (result->value && !gc_neutral_expression(*result->value))
            {
                return false;
            }
        }
        else if (const auto* branch = std::get_if<if_statement>(&item->data))
        {
            if (!gc_neutral_expression(*branch->condition) ||
                !gc_neutral_body(branch->then_body) ||
                !gc_neutral_body(branch->else_body))
            {
                return false;
            }
        }
        else if (const auto* loop = std::get_if<while_statement>(&item->data))
        {
            if (!gc_neutral_expression(*loop->condition) ||
                !gc_neutral_body(loop->body))
            {
                return false;
            }
        }
        else if (const auto* loop = std::get_if<for_loop>(&item->data))
        {
            if (!gc_neutral_expression(*loop->first) ||
                !gc_neutral_expression(*loop->last) ||
                !gc_neutral_body(loop->body))
            {
                return false;
            }
        }
        else if (const auto* loop = std::get_if<for_each>(&item->data))
        {
            if (!gc_neutral_expression(*loop->values) ||
                !gc_neutral_body(loop->body))
            {
                return false;
            }
        }
        else if (!gc_neutral_statement(*item))
        {
            return false;
        }
    }
    return true;
}

bool llvm_code_generator::gc_neutral_function(const function_decl& function)
{
    if (function.external)
    {
        // 外部调用按已绑定调用点的属性判断；没有调用点时保持保守。
        return false;
    }
    if (const auto found = gc_neutral_cache_.find(&function);
        found != gc_neutral_cache_.end())
    {
        return found->second;
    }
    if (!gc_visiting_.insert(&function).second)
    {
        return false;
    }
    const bool result = gc_neutral_body(function.body);
    gc_visiting_.erase(&function);
    gc_neutral_cache_.emplace(&function, result);
    return result;
}

} // namespace tx
