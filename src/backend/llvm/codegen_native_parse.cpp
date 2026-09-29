#include "backend/llvm/codegen.hpp"

#include <algorithm>

namespace tx
{

const call_expression* llvm_code_generator::native_parse_initializer(
    const variable_declaration& declaration) const
{
    if (declaration.array_length || !declaration.initializer)
    {
        return nullptr;
    }
    const auto* call = std::get_if<call_expression>(&declaration.initializer->data);
    if (!call || call->indirect || call->receiver || !call->overload_index)
    {
        return nullptr;
    }
    const auto found = functions_.find(call->name);
    if (found == functions_.end() || *call->overload_index >= found->second.size())
    {
        return nullptr;
    }
    const auto& target = *found->second[*call->overload_index];
    if (!target.external || (target.external_name != "parse.try_parse_int" &&
                             target.external_name != "parse.try_parse_float") ||
        declaration.initializer->type != target.return_type ||
        (declaration.declared_type && *declaration.declared_type != target.return_type) ||
        call->arguments.size() != target.parameters.size())
    {
        return nullptr;
    }
    for (std::size_t index = 0; index < call->arguments.size(); ++index)
    {
        if (call->arguments[index].kind != argument_kind::positional ||
            call->arguments[index].value->type != target.parameters[index].type)
        {
            return nullptr;
        }
    }
    return call;
}

bool llvm_code_generator::is_parse_result_type(const value_type& type) const
{
    if (!structs_.contains(type.name))
    {
        return false;
    }
    for (const auto& [name, overloads] : functions_)
    {
        for (const auto* target : overloads)
        {
            if (target->external && target->return_type == type &&
                (target->external_name == "parse.try_parse_int" ||
                 target->external_name == "parse.try_parse_float"))
            {
                return true;
            }
        }
    }
    return false;
}

bool llvm_code_generator::gc_neutral_native_parse_declaration(const call_expression& call)
{
    for (std::size_t index = 0; index < call.arguments.size(); ++index)
    {
        const auto& argument = *call.arguments[index].value;
        if (argument.type != value_type::str_type)
        {
            if (!gc_neutral_expression(argument))
            {
                return false;
            }
            continue;
        }
        if (index >= call.properties.arguments.size() ||
            call.properties.arguments[index] != argument_ownership::borrowed ||
            !call.properties.effects.allows_borrow() ||
            !std::all_of(call.arguments.begin() + index + 1, call.arguments.end(),
                [](const call_argument& following)
                {
                    return stable_value_expression(*following.value);
                }))
        {
            return false;
        }
        if (std::holds_alternative<name_reference>(argument.data))
        {
            continue;
        }
        // 与只读文本生成的向量槽借用对应；复杂拥有者仍按保守路径轮询。
        const auto* access = std::get_if<index_expression>(&argument.data);
        if (!access || access->object->type != value_type::vector_of(value_type::str_type) ||
            !std::holds_alternative<name_reference>(access->object->data) ||
            !stable_value_expression(*access->index) || !gc_neutral_expression(*access->index))
        {
            return false;
        }
    }
    return true;
}

bool llvm_code_generator::emit_native_parse_declaration(
    const statement& item, const variable_declaration& declaration)
{
    const auto* call = native_parse_initializer(declaration);
    if (!call)
    {
        return false;
    }
    const auto& type = declaration.initializer->type;
    const auto& scalar_type = structs_.at(type.name)->fields.at(1).type;
    variable_slot variable{type, allocate(type, item.position)};
    variable.native_parse_ok = allocate(value_type::bool_type, item.position);
    variable.native_parse_value = allocate(scalar_type, item.position);
    variable.native_parse_error = allocate(value_type::int_type, item.position);
    // 槽位每次执行声明时重置；循环再次进入时不能沿用上轮的物化状态。
    write_instruction("store ptr null, ptr " + variable.address);
    std::vector<ir_value> arguments;
    for (std::size_t index = 0; index < call->arguments.size(); ++index)
    {
        arguments.push_back(call_argument_value(*call, index));
    }
    const bool integer = scalar_type == value_type::int_type;
    const auto status = temporary();
    write_instruction(status + " = call i32 @txrt_parse_" +
        std::string(integer ? "int" : "float") + "_scalar(ptr " + arguments[0].text +
        (integer ? ", i64 " + (arguments.size() == 2 ? arguments[1].text : "10") : "") +
        ", ptr " + variable.native_parse_ok + ", ptr " + variable.native_parse_value +
        ", ptr " + variable.native_parse_error + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    for (const auto& argument : arguments)
    {
        release(argument);
    }
    scopes_.back().emplace(declaration.name, std::move(variable));
    return true;
}

void llvm_code_generator::materialize_native_parse(const variable_slot& variable)
{
    if (variable.native_parse_ok.empty())
    {
        return;
    }
    const auto handle = temporary();
    const auto missing = temporary();
    const auto create = label();
    const auto ready = label();
    write_instruction(handle + " = load ptr, ptr " + variable.address);
    write_instruction(missing + " = icmp eq ptr " + handle + ", null");
    write_instruction("br i1 " + missing + ", label %" + create + ", label %" + ready);
    start_block(create);
    const auto& fields = structs_.at(variable.type.name)->fields;
    const auto& scalar_type = fields.at(1).type;
    const auto ok = load({value_type::bool_type, variable.native_parse_ok});
    const auto scalar = load({scalar_type, variable.native_parse_value});
    const auto error = load({value_type::int_type, variable.native_parse_error});
    const auto status = temporary();
    // 始终在原局部根中提交物化对象，所有别名和控制流分支观察同一身份。
    write_instruction(status + " = call i32 @txrt_parse_materialize_" +
        std::string(scalar_type == value_type::int_type ? "int" : "float") +
        "(i1 " + ok.text + ", " + llvm_type(scalar_type, {}) + " " + scalar.text +
        ", i64 " + error.text + ", ptr " + global_bytes(variable.type.name) +
        ", ptr " + global_bytes(fields.at(2).type.name) +
        ", ptr " + variable.address + ")");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
    write_instruction("br label %" + ready);
    start_block(ready);
}

std::string llvm_code_generator::native_parse_field_address(
    const variable_slot& variable, const expression& item)
{
    const auto& member = std::get<member_expression>(item.data);
    const auto handle = temporary();
    const auto missing = temporary();
    const auto local = label();
    const auto boxed = label();
    const auto ready = label();
    write_instruction(handle + " = load ptr, ptr " + variable.address);
    write_instruction(missing + " = icmp eq ptr " + handle + ", null");
    write_instruction("br i1 " + missing + ", label %" + local + ", label %" + boxed);
    start_block(local);
    write_instruction("br label %" + ready);
    start_block(boxed);
    const auto field = temporary();
    const auto* suffix = item.type == value_type::bool_type ? "bool"
        : item.type == value_type::int_type ? "i64" : "f64";
    write_instruction(field + " = call ptr @txrt_struct_field_" + suffix +
        "_ptr(ptr " + handle + ", i64 " + (member.field == "ok" ? "0" : "1") + ")");
    // 快速 ABI 在 try 内会追加错误检查块，phi 的前驱必须取检查后的块。
    const auto boxed_end = label();
    write_instruction("br label %" + boxed_end);
    start_block(boxed_end);
    write_instruction("br label %" + ready);
    start_block(ready);
    const auto address = temporary();
    write_instruction(address + " = phi ptr [ " +
        (member.field == "ok" ? variable.native_parse_ok : variable.native_parse_value) +
        ", %" + local + " ], [ " + field + ", %" + boxed_end + " ]");
    return address;
}

} // namespace tx
