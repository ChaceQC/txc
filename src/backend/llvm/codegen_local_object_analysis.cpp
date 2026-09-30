#include "backend/llvm/codegen.hpp"

#include <algorithm>
#include <functional>

namespace tx
{
namespace
{

// 只允许指定局部的字段操作或容器方法；作为普通实参、别名或返回值均视为逃逸。
class local_use_checker
{
public:
    const variable_declaration& candidate;
    bool fields_only;
    bool scalar_reads = false;
    bool read_only_fields = false;
    const variable_declaration* allowed_alias = nullptr;
    bool reject_returns = false;
    std::function<bool(const call_expression&)> native_call = {};
    std::function<bool(const operator_binding&)> native_operator = {};
    std::function<bool(const expression&)> native_result = {};
    std::function<bool(const call_expression&)> guard_call = {};
    const function_analysis* analysis = nullptr;
    bool length_only = false;
    bool reject_super = false;

    bool field_owner(const expression& item) const
    {
        if (const auto* member = std::get_if<member_expression>(&item.data))
        {
            return field_owner(*member->object);
        }
        return named(item);
    }

    bool named(const expression& item) const
    {
        const auto* name = std::get_if<name_reference>(&item.data);
        if (!name || name->name != candidate.name)
        {
            return false;
        }
        if (analysis)
        {
            const auto found = analysis->ir.local_bindings.find(&item);
            if (found != analysis->ir.local_bindings.end())
            {
                return analysis->ir.variables[found->second].declaration == &candidate;
            }
        }
        // 未降下来的不可达语句仍按名字保守检查。
        return true;
    }

    bool expression_safe(const expression& item) const
    {
        if (named(item))
        {
            return scalar_reads;
        }
        if (const auto* member = std::get_if<member_expression>(&item.data))
        {
            if (length_only)
            {
                return expression_safe(*member->object);
            }
            if (read_only_fields && field_owner(*member->object))
            {
                return item.type == value_type::int_type || item.type == value_type::float_type ||
                    item.type == value_type::bool_type || item.type == value_type::str_type;
            }
            return (fields_only && named(*member->object)) ||
                expression_safe(*member->object);
        }
        if (const auto* call = std::get_if<call_expression>(&item.data))
        {
            if (reject_super && call->is_super_view)
            {
                return false;
            }
            if (length_only && !call->receiver && !call->indirect && !call->overload_index &&
                call->name == "len" && call->arguments.size() == 1 && named(*call->arguments.front().value))
            {
                return true;
            }
            if (guard_call && guard_call(*call))
            {
                return !call->receiver && std::all_of(call->arguments.begin(), call->arguments.end(),
                    [&](const call_argument& argument)
                    {
                        return (&argument == &call->arguments.front() && named(*argument.value)) ||
                            expression_safe(*argument.value);
                    });
            }
            if (native_call && native_call(*call))
            {
                return (!call->receiver || named(*call->receiver) || expression_safe(*call->receiver)) &&
                    std::all_of(call->arguments.begin(), call->arguments.end(), [&](const call_argument& argument)
                    {
                        return named(*argument.value) || expression_safe(*argument.value);
                    });
            }
            if (scalar_reads && call->name == "move" &&
                std::any_of(call->arguments.begin(), call->arguments.end(),
                    [&](const call_argument& argument)
                    {
                        return named(*argument.value);
                    }))
            {
                return false;
            }
            if (call->receiver && !(!fields_only && named(*call->receiver)) &&
                !expression_safe(*call->receiver))
            {
                return false;
            }
            return std::all_of(call->arguments.begin(), call->arguments.end(),
                [&](const call_argument& argument)
                {
                    return expression_safe(*argument.value);
                });
        }
        if (const auto* binary = std::get_if<binary_operation>(&item.data))
        {
            if (binary->binding && native_operator && native_operator(*binary->binding))
            {
                return (named(*binary->left) || expression_safe(*binary->left)) &&
                    (named(*binary->right) || expression_safe(*binary->right));
            }
            return expression_safe(*binary->left) && expression_safe(*binary->right);
        }
        if (const auto* unary = std::get_if<unary_operation>(&item.data))
        {
            return expression_safe(*unary->operand);
        }
        if (const auto* update = std::get_if<update_expression>(&item.data))
        {
            if (read_only_fields && field_owner(*update->target))
            {
                return false;
            }
            return !(scalar_reads && named(*update->target)) && expression_safe(*update->target);
        }
        if (const auto* cast = std::get_if<cast_expression>(&item.data))
        {
            return expression_safe(*cast->value);
        }
        if (const auto* index = std::get_if<index_expression>(&item.data))
        {
            return expression_safe(*index->object) && expression_safe(*index->index);
        }
        if (const auto* array = std::get_if<array_literal>(&item.data))
        {
            return std::all_of(array->elements.begin(), array->elements.end(),
                [&](const expr_ptr& value)
                {
                    return expression_safe(*value);
                });
        }
        if (const auto* dictionary = std::get_if<dictionary_literal>(&item.data))
        {
            return std::all_of(dictionary->entries.begin(), dictionary->entries.end(),
                [&](const dictionary_entry& entry)
                {
                    return expression_safe(*entry.key) && expression_safe(*entry.value);
                });
        }
        return true;
    }

    bool body_safe(const std::vector<stmt_ptr>& body) const
    {
        return std::all_of(body.begin(), body.end(), [&](const stmt_ptr& item)
        {
            return statement_safe(*item);
        });
    }

    bool statement_safe(const statement& item) const
    {
        if (const auto* declaration = std::get_if<variable_declaration>(&item.data))
        {
            return (analysis || declaration == &candidate || declaration->name != candidate.name) &&
                (declaration == allowed_alias || !declaration->initializer ||
                 expression_safe(*declaration->initializer)) &&
                (!declaration->array_length || expression_safe(*declaration->array_length));
        }
        if (const auto* assignment = std::get_if<variable_assignment>(&item.data))
        {
            if (native_result && named(*assignment->target))
            {
                return (assignment->binding ? native_operator(*assignment->binding)
                    : assignment->operation == token_kind::equal && native_result(*assignment->value)) &&
                    (named(*assignment->value) || expression_safe(*assignment->value));
            }
            if (read_only_fields && field_owner(*assignment->target))
            {
                return false;
            }
            return !(scalar_reads && named(*assignment->target)) &&
                expression_safe(*assignment->target) && expression_safe(*assignment->value);
        }
        if (const auto* unpack = std::get_if<unpack_assignment>(&item.data))
        {
            if (analysis)
            {
                const auto found = analysis->ir.unpack_bindings.find(unpack);
                if (found != analysis->ir.unpack_bindings.end())
                {
                    return std::none_of(found->second.begin(), found->second.end(), [&](analysis_id id)
                    {
                        return analysis->ir.variables[id].declaration == &candidate;
                    }) && expression_safe(*unpack->value);
                }
            }
            return std::find(unpack->names.begin(), unpack->names.end(), candidate.name) ==
                unpack->names.end() && expression_safe(*unpack->value);
        }
        if (const auto* branch = std::get_if<if_statement>(&item.data))
        {
            return expression_safe(*branch->condition) &&
                body_safe(branch->then_body) && body_safe(branch->else_body);
        }
        if (const auto* loop = std::get_if<while_statement>(&item.data))
        {
            return expression_safe(*loop->condition) && body_safe(loop->body);
        }
        if (const auto* loop = std::get_if<for_loop>(&item.data))
        {
            return (analysis || loop->name != candidate.name) && expression_safe(*loop->first) &&
                expression_safe(*loop->last) && body_safe(loop->body);
        }
        if (const auto* loop = std::get_if<for_each>(&item.data))
        {
            return (analysis || loop->name != candidate.name) && expression_safe(*loop->values) &&
                body_safe(loop->body);
        }
        if (const auto* guarded = std::get_if<try_statement>(&item.data))
        {
            return body_safe(guarded->body) && std::all_of(
                guarded->handlers.begin(), guarded->handlers.end(),
                [&](const exception_clause& handler)
                {
                    return (analysis || handler.name != candidate.name) && body_safe(handler.body);
                });
        }
        if (const auto* result = std::get_if<return_statement>(&item.data))
        {
            return !reject_returns && (!result->value || expression_safe(*result->value));
        }
        return expression_safe(*std::get<expression_statement>(item.data).value);
    }
};

} // namespace

bool llvm_code_generator::confined_local(
    const variable_declaration& declaration, bool fields_only) const
{
    if (current_analysis_ && !current_analysis_->confined(declaration))
    {
        return false;
    }
    auto checker = local_use_checker{declaration, fields_only};
    checker.analysis = current_analysis_;
    if (fields_only && declaration.initializer && scalar_record_type(declaration.initializer->type))
    {
        checker.native_call = [this](const call_expression& call)
        {
            return native_record_target(call) != nullptr;
        };
        checker.native_operator = [this](const operator_binding& binding)
        {
            return native_record_target(binding) != nullptr;
        };
        checker.native_result = [this](const expression& value)
        {
            return native_record_expression(value);
        };
    }
    return current_function_body_ && checker.body_safe(*current_function_body_);
}

bool llvm_code_generator::borrowed_class_local(const variable_declaration& declaration) const
{
    auto checker = local_use_checker{declaration, false, true};
    checker.analysis = current_analysis_;
    // 非异常模式的显式 return 从外层开始清理；此时保留拥有的转换结果。
    checker.reject_returns = !recoverable_errors_;
    return current_function_body_ && checker.body_safe(*current_function_body_);
}

bool llvm_code_generator::confined_guard_local(const variable_declaration& declaration) const
{
    auto checker = local_use_checker{declaration, true};
    checker.analysis = current_analysis_;
    checker.guard_call = [this](const call_expression& call)
    {
        const auto found = functions_.find(call.name);
        if (call.indirect || call.receiver || !call.overload_index || found == functions_.end() ||
            *call.overload_index >= found->second.size())
        {
            return false;
        }
        const auto& target = *found->second[*call.overload_index];
        return target.external && (target.external_name == "sync.guard_get" ||
            target.external_name == "sync.guard_set" || target.external_name == "sync.guard_close");
    };
    return current_function_body_ && checker.body_safe(*current_function_body_);
}

bool llvm_code_generator::scalar_local_unchanged(const std::string& name,
    const std::vector<stmt_ptr>& body, const variable_declaration* allowed)
{
    variable_declaration candidate;
    candidate.name = name;
    return local_use_checker{allowed ? *allowed : candidate, false, true}.body_safe(body);
}

bool llvm_code_generator::readonly_local_fields(const variable_declaration& declaration,
    const variable_declaration* allowed_alias) const
{
    auto checker = local_use_checker{declaration, true, false, true, allowed_alias};
    checker.analysis = current_analysis_;
    return current_function_body_ && checker.body_safe(*current_function_body_);
}

bool llvm_code_generator::default_heap_local(const variable_declaration& declaration) const
{
    const auto* call = declaration.initializer
        ? std::get_if<call_expression>(&declaration.initializer->data) : nullptr;
    if (!call || !call->container_type || call->container_type->container_name() != "heap")
    {
        return false;
    }
    const auto& element = call->container_type->parameters.front();
    return (element == value_type::int_type || element == value_type::float_type ||
            element == value_type::bool_type || element == value_type::str_type) &&
        std::none_of(call->arguments.begin(), call->arguments.end(),
            [](const call_argument& argument)
            {
                return argument.value->type.is_function();
            }) && confined_local(declaration, false);
}

bool llvm_code_generator::length_only_local(const variable_declaration& declaration) const
{
    auto checker = local_use_checker{declaration, false};
    checker.analysis = current_analysis_;
    checker.length_only = true;
    // 禁止任何接收者调用，只有 len 的专用分支可以读取候选值。
    checker.fields_only = true;
    checker.read_only_fields = true;
    return current_function_body_ && checker.body_safe(*current_function_body_);
}

bool llvm_code_generator::field_only_local(const variable_declaration& declaration) const
{
    auto checker = local_use_checker{declaration, true};
    checker.analysis = current_analysis_;
    return current_function_body_ && checker.body_safe(*current_function_body_);
}

bool llvm_code_generator::field_only_self(const function_decl& function)
{
    variable_declaration self;
    self.name = "self";
    auto checker = local_use_checker{self, true};
    checker.reject_super = true;
    return !function.external && checker.body_safe(function.body);
}

bool llvm_code_generator::default_heap_receiver(const call_expression& call) const
{
    const auto* name = call.receiver
        ? std::get_if<name_reference>(&call.receiver->data) : nullptr;
    return name && find_variable(name->name, call.receiver->position).default_heap &&
        (call.name == "push" || call.name == "top" || call.name == "pop");
}

} // namespace tx
