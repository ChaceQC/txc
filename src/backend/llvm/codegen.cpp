#include "backend/llvm/codegen.hpp"

#include <algorithm>
#include <iomanip>
#include <utility>

namespace tx
{

std::string llvm_code_generator::llvm_type(const value_type& type,
                                           source_pos position)
{
    if (type == value_type::int_type)
    {
        return "i64";
    }
    if (type == value_type::bool_type)
    {
        return "i1";
    }
    if (type == value_type::float_type)
    {
        return "double";
    }
    if (type == value_type::str_type)
    {
        return "ptr";
    }
    if (is_value_handle(type))
    {
        return "ptr";
    }
    if (type == value_type::void_type)
    {
        return "void";
    }
    throw compile_error(position, "LLVM 后端暂不支持类型：" + type.name);
}

bool llvm_code_generator::is_value_handle(const value_type& type)
{
    return type != value_type::int_type && type != value_type::bool_type &&
           type != value_type::float_type && type != value_type::str_type &&
           type != value_type::void_type && type != value_type::unknown_type;
}

std::string llvm_code_generator::function_name(const std::string& name,
                                               std::size_t overload)
{
    return "@tx_fn_" + name + "_" + std::to_string(overload);
}

std::string llvm_code_generator::temporary()
{
    return "%t" + std::to_string(next_value_++);
}

std::string llvm_code_generator::label()
{
    return "block" + std::to_string(next_label_++);
}

std::string llvm_code_generator::allocate(const value_type& type,
                                          source_pos position)
{
    const auto address = "%slot" + std::to_string(next_slot_++);
    allocations_ << "  " << address << " = alloca "
                 << llvm_type(type, position) << '\n';
    return address;
}

std::string llvm_code_generator::global_bytes(std::string_view bytes)
{
    const auto name = "@tx_literal_" + std::to_string(next_string_++);
    globals_ << name << " = private constant [" << bytes.size() + 1
             << " x i8] c\"";
    for (const unsigned char byte : bytes)
    {
        globals_ << '\\' << std::hex << std::uppercase << std::setw(2)
                 << std::setfill('0') << static_cast<int>(byte) << std::dec;
    }
    globals_ << "\\00\"\n";
    return name;
}

llvm_code_generator::variable_slot llvm_code_generator::find_variable(
    const std::string& name, source_pos position) const
{
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope)
    {
        if (const auto found = scope->find(name); found != scope->end())
        {
            return found->second;
        }
    }
    throw compile_error(position, "LLVM 后端找不到变量：" + name);
}

llvm_code_generator::ir_value llvm_code_generator::load(
    const variable_slot& variable)
{
    const auto result = temporary();
    write_instruction(result + " = load " +
                      llvm_type(variable.type, {}) + ", ptr " + variable.address);
    if (variable.type == value_type::str_type)
    {
        const auto copied = allocate(value_type::str_type, {});
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_str_clone(ptr " + result +
                          ", ptr " + copied + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        const auto cloned = temporary();
        write_instruction(cloned + " = load ptr, ptr " + copied);
        return {variable.type, cloned};
    }
    if (is_value_handle(variable.type))
    {
        const auto copied = allocate(variable.type, {});
        const auto status = temporary();
        write_instruction(status + " = call i32 @txrt_value_clone(ptr " +
                          result + ", ptr " + copied + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        const auto cloned = temporary();
        write_instruction(cloned + " = load ptr, ptr " + copied);
        return {variable.type, cloned};
    }
    return {variable.type, result};
}

void llvm_code_generator::release(const ir_value& value)
{
    if (value.type == value_type::str_type)
    {
        write_instruction("call void @txrt_str_release(ptr " + value.text + ")");
    }
    else if (is_value_handle(value.type))
    {
        write_instruction("call void @txrt_value_release(ptr " + value.text + ")");
    }
}

void llvm_code_generator::release_slot(const variable_slot& variable)
{
    if (variable.type == value_type::str_type ||
        is_value_handle(variable.type))
    {
        const auto held = temporary();
        write_instruction(held + " = load ptr, ptr " + variable.address);
        release({variable.type, held});
    }
}

void llvm_code_generator::write_instruction(const std::string& text)
{
    body_ << "  " << text << '\n';
}

void llvm_code_generator::start_block(const std::string& name)
{
    body_ << name << ":\n";
    terminated_ = false;
}

void llvm_code_generator::push_scope()
{
    scopes_.emplace_back();
}

void llvm_code_generator::pop_scope()
{
    if (!terminated_)
    {
        for (const auto& [name, variable] : scopes_.back())
        {
            (void)name;
            release_slot(variable);
        }
    }
    scopes_.pop_back();
}

void llvm_code_generator::emit_function(const function_decl& function)
{
    if (function.external)
    {
        return;
    }
    const auto& overloads = functions_.at(function.name);
    const auto found = std::find(overloads.begin(), overloads.end(), &function);
    const auto index = static_cast<std::size_t>(found - overloads.begin());
    return_type_ = function.return_type;
    allocations_.str({});
    allocations_.clear();
    body_.str({});
    body_.clear();
    scopes_.clear();
    next_value_ = 0;
    next_slot_ = 0;
    next_label_ = 0;
    terminated_ = false;
    push_scope();

    std::string parameters;
    for (std::size_t i = 0; i < function.parameters.size(); ++i)
    {
        const auto& parameter = function.parameters[i];
        if (i != 0)
        {
            parameters += ", ";
        }
        const auto type = llvm_type(parameter.type, parameter.position);
        parameters += type + " %arg" + std::to_string(i);
        const auto address = allocate(parameter.type, parameter.position);
        write_instruction("store " + type + " %arg" + std::to_string(i) +
                          ", ptr " + address);
        scopes_.back().emplace(parameter.name,
                               variable_slot{parameter.type, address});
    }
    emit_statements(function.body);
    if (!terminated_)
    {
        for (const auto& [name, variable] : scopes_.back())
        {
            (void)name;
            release_slot(variable);
        }
        write_instruction(function.return_type == value_type::void_type
            ? "ret void" : "unreachable");
        terminated_ = true;
    }
    pop_scope();

    module_ << "define " << llvm_type(function.return_type, function.position)
            << ' ' << function_name(function.name, index) << '(' << parameters
            << ") {\nentry:\n" << allocations_.str() << body_.str() << "}\n\n";
}

std::string llvm_code_generator::generate(const program& source)
{
    functions_.clear();
    structs_.clear();
    module_.str({});
    module_.clear();
    globals_.str({});
    globals_.clear();
    next_string_ = 0;
    for (const auto& function : source.functions)
    {
        functions_[function.name].push_back(&function);
    }
    for (const auto& definition : source.structs)
    {
        structs_.emplace(definition.name, &definition);
    }
    module_ << "target triple = \"x86_64-w64-windows-gnu\"\n\n"
            << "declare i32 @txrt_prepare_console()\n"
            << "declare void @txrt_require_success(i32)\n"
            << "declare i32 @txrt_print_i64(i64)\n"
            << "declare i32 @txrt_print_f64(double)\n"
            << "declare i32 @txrt_print_bool(i1)\n"
            << "declare i32 @txrt_exit_code(i64)\n"
            << "declare i32 @txrt_float_to_int(double, ptr)\n"
            << "declare i32 @txrt_add_i64(i64, i64, ptr)\n"
            << "declare i32 @txrt_sub_i64(i64, i64, ptr)\n"
            << "declare i32 @txrt_mul_i64(i64, i64, ptr)\n"
            << "declare i32 @txrt_div_i64(i64, i64, ptr)\n"
            << "declare i32 @txrt_neg_i64(i64, ptr)\n"
            << "declare i32 @txrt_div_f64(double, double, ptr)\n\n";
    module_ << "declare i32 @txrt_str_new(ptr, i64, ptr)\n"
            << "declare i32 @txrt_str_clone(ptr, ptr)\n"
            << "declare void @txrt_str_release(ptr)\n"
            << "declare i32 @txrt_str_concat(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_str_compare(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_str_len(ptr, ptr)\n"
            << "declare i32 @txrt_print_str(ptr)\n"
            << "declare i32 @txrt_input(ptr, ptr)\n"
            << "declare i32 @txrt_parse_int(ptr, ptr)\n"
            << "declare i32 @txrt_parse_float(ptr, ptr)\n"
            << "declare i32 @txrt_int_to_str(i64, ptr)\n"
            << "declare i32 @txrt_float_to_str(double, ptr)\n"
            << "declare i32 @txrt_bool_to_str(i1, ptr)\n\n";
    module_ << "declare i32 @txrt_value_none(ptr)\n"
            << "declare i32 @txrt_value_box_i64(i64, ptr)\n"
            << "declare i32 @txrt_value_box_f64(double, ptr)\n"
            << "declare i32 @txrt_value_box_bool(i1, ptr)\n"
            << "declare i32 @txrt_value_box_str(ptr, ptr)\n"
            << "declare i32 @txrt_value_clone(ptr, ptr)\n"
            << "declare void @txrt_value_release(ptr)\n"
            << "declare i32 @txrt_value_assign(ptr, ptr)\n"
            << "declare i32 @txrt_value_to_i64(ptr, ptr)\n"
            << "declare i32 @txrt_value_to_f64(ptr, ptr)\n"
            << "declare i32 @txrt_value_to_bool(ptr, ptr)\n"
            << "declare i32 @txrt_value_to_str(ptr, ptr)\n"
            << "declare i32 @txrt_value_is_none(ptr, ptr)\n"
            << "declare i32 @txrt_value_len(ptr, ptr)\n"
            << "declare i32 @txrt_value_print(ptr)\n"
            << "declare i32 @txrt_value_require_type(ptr, ptr)\n"
            << "declare i32 @txrt_array_new(i64, ptr)\n"
            << "declare i32 @txrt_array_resize(i64, ptr, ptr)\n"
            << "declare i32 @txrt_array_len(ptr, ptr)\n"
            << "declare i32 @txrt_array_element_address(ptr, i64, ptr)\n"
            << "declare i32 @txrt_array_append(ptr, ptr)\n"
            << "declare i32 @txrt_array_extend(ptr, ptr)\n"
            << "declare i32 @txrt_array_require_length(ptr, i64)\n"
            << "declare i32 @txrt_dict_new(ptr)\n"
            << "declare i32 @txrt_dict_set(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_dict_len(ptr, ptr)\n"
            << "declare i32 @txrt_dict_key_address(ptr, i64, ptr)\n"
            << "declare i32 @txrt_value_element_address(ptr, ptr, i1, ptr)\n"
            << "declare i32 @txrt_keyword_set(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_keyword_merge(ptr, ptr)\n"
            << "declare i32 @txrt_call_bind(ptr, ptr, ptr, i64, i32, i32, ptr)\n"
            << "declare i32 @txrt_struct_new(ptr, i64, ptr)\n"
            << "declare i32 @txrt_struct_set_field(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_struct_field_address(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_call_external(ptr, ptr, i64, ptr)\n\n";

    for (const auto& function : source.functions)
    {
        emit_function(function);
    }
    module_ << "define i32 @main() {\nentry:\n"
            << "  %prepare = call i32 @txrt_prepare_console()\n"
            << "  call void @txrt_require_success(i32 %prepare)\n"
            << "  %result = call i64 " << function_name("main", 0) << "()\n"
            << "  %exit = call i32 @txrt_exit_code(i64 %result)\n"
            << "  ret i32 %exit\n}\n";
    module_ << globals_.str();
    return module_.str();
}

} // namespace tx
