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
    if (type.is_function())
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

value_type llvm_code_generator::parameter_abi_type(const parameter& value)
{
    return parameter_is_nullable(value) ? value_type::any_type : value.type;
}

bool llvm_code_generator::is_value_handle(const value_type& type)
{
    return type != value_type::int_type && type != value_type::bool_type &&
           type != value_type::float_type && type != value_type::str_type &&
           type != value_type::void_type && type != value_type::unknown_type &&
           !type.is_inferred_function();
}

std::string llvm_code_generator::function_name(const std::string& name,
                                               std::size_t overload)
{
    return "@tx_fn_" + name + "_" + std::to_string(overload);
}

std::string llvm_code_generator::callback_name(const std::string& name,
                                               std::size_t overload)
{
    return "@tx_callback_" + name + "_" + std::to_string(overload);
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
                                          source_pos position, bool owned)
{
    const auto address = "%slot" + std::to_string(next_slot_++);
    allocations_ << "  " << address << " = alloca "
                 << llvm_type(type, position) << '\n';
    if (recoverable_errors_ && owned &&
        (type == value_type::str_type || is_value_handle(type)))
    {
        allocations_ << "  store ptr null, ptr " << address << '\n';
        error_root_indices_.emplace(address, error_roots_.size());
        error_roots_.push_back({type, address, scopes_.size()});
    }
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
    if (name == "super" && !current_method_owner_.empty())
    {
        for (const auto& base : classes_.at(current_method_owner_)->bases)
        {
            if (!classes_.at(base)->is_interface)
            {
                return {value_type(base), scopes_.front().at("self").address};
            }
        }
    }
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope)
    {
        if (const auto found = scope->find(name); found != scope->end())
        {
            return found->second;
        }
    }
    throw compile_error(position, "LLVM 后端找不到变量：" + name);
}

std::size_t llvm_code_generator::field_index(const value_type& type,
                                              std::string_view field,
                                              source_pos position) const
{
    if (type.is_entry())
    {
        if (field == "key")
        {
            return 0;
        }
        if (field == "value")
        {
            return 1;
        }
    }
    if (type.is_selected())
    {
        if (field == "index")
        {
            return 0;
        }
        if (field == "value")
        {
            return 1;
        }
    }
    if (type.is_priority_entry())
    {
        if (field == "priority")
        {
            return 0;
        }
        if (field == "value")
        {
            return 1;
        }
    }
    const auto found = structs_.find(type.name);
    if (found != structs_.end())
    {
        const auto& fields = found->second->fields;
        for (std::size_t index = 0; index < fields.size(); ++index)
        {
            if (fields[index].name == field)
            {
                return index;
            }
        }
    }
    throw compile_error(position, "LLVM 后端找不到结构体字段：" +
                                  std::string(field));
}

llvm_code_generator::ir_value llvm_code_generator::load(
    const variable_slot& variable)
{
    if (!variable.native_option_value.empty())
    {
        return load_native_option(variable);
    }
    if (!variable.snapshot_kind.empty())
    {
        const auto kind = temporary();
        write_instruction(kind + " = load i8, ptr " + variable.snapshot_kind);
        const auto bits = temporary();
        write_instruction(bits + " = load i64, ptr " + variable.snapshot_bits);
        const auto fallback = temporary();
        write_instruction(fallback + " = load ptr, ptr " + variable.address);
        const auto output = allocate(value_type::any_type, {});
        const auto status = temporary();
        write_instruction(status +
            " = call i32 @txrt_value_snapshot_box(i8 " + kind +
            ", i64 " + bits + ", ptr " + fallback + ", ptr " + output + ")");
        write_instruction("call void @txrt_require_success(i32 " + status + ")");
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + output);
        return {variable.type, result};
    }
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
        return {variable.type, cloned, false, load_vector_reference(variable)};
    }
    return {variable.type, result};
}

std::string llvm_code_generator::cache_array_reference(
    const std::string& handle, source_pos position)
{
    const auto address = allocate(value_type::array_type, position, false);
    const auto reference = temporary();
    write_instruction(reference + " = call ptr @txrt_array_ref(ptr " +
                      handle + ")");
    write_instruction("store ptr " + reference + ", ptr " + address);
    return address;
}

void llvm_code_generator::refresh_array_reference(
    const variable_slot& variable, const std::string& handle)
{
    if (variable.array_reference.empty())
    {
        return;
    }
    const auto reference = temporary();
    write_instruction(reference + " = call ptr @txrt_array_ref(ptr " +
                      handle + ")");
    write_instruction("store ptr " + reference + ", ptr " +
                      variable.array_reference);
}

std::string llvm_code_generator::load_array_reference(
    const variable_slot& variable)
{
    const auto result = temporary();
    write_instruction(result + " = load ptr, ptr " + variable.array_reference);
    return result;
}

llvm_code_generator::ir_value llvm_code_generator::expression_value_or_borrow(
    const expression& item, bool& borrowed)
{
    const name_reference* name = std::get_if<name_reference>(&item.data);
    if (const auto* call = std::get_if<call_expression>(&item.data);
        call && call->is_super_view)
    {
        name = nullptr;
        const auto variable = find_variable("self", item.position);
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + variable.address);
        borrowed = true;
        return {item.type, result};
    }
    if (name && !name->function_value && (is_value_handle(item.type) ||
                 item.type == value_type::str_type))
    {
        const auto variable = find_variable(name->name, item.position);
        if (!variable.snapshot_kind.empty() ||
            !variable.native_option_value.empty())
        {
            borrowed = false;
            return load(variable);
        }
        const auto result = temporary();
        write_instruction(result + " = load ptr, ptr " + variable.address);
        borrowed = true;
        return {item.type, result, false, load_vector_reference(variable)};
    }
    borrowed = false;
    return expression_value(item);
}

void llvm_code_generator::release(const ir_value& value)
{
    if (value.borrowed)
    {
        return;
    }
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
    if (!variable.native_option_value.empty())
    {
        return;
    }
    if (!variable.dynamic_array_length.empty())
    {
        write_instruction("call void @txrt_local_scalar_array_release(ptr " +
                          variable.address + ")");
        return;
    }
    if (variable.local_array_length)
    {
        return;
    }
    if (!variable.snapshot_kind.empty())
    {
        const auto fallback = temporary();
        write_instruction(fallback + " = load ptr, ptr " + variable.address);
        write_instruction("call void @txrt_value_release(ptr " + fallback + ")");
        return;
    }
    if (variable.borrowed)
    {
        return;
    }
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
    constexpr std::string_view check = "call void @txrt_require_success(i32 ";
    if (text.starts_with(check) && text.ends_with(')'))
    {
        const auto status = text.substr(check.size(),
                                        text.size() - check.size() - 1);
        if (recoverable_errors_)
        {
            emit_error_check(status);
            return;
        }
        const auto failed = temporary();
        body_ << "  " << failed << " = icmp ne i32 " << status << ", 0\n";
        const auto error_label = label();
        const auto success_label = label();
        body_ << "  br i1 " << failed << ", label %" << error_label
              << ", label %" << success_label << '\n';
        start_block(error_label);
        emit_error_location();
        body_ << "  " << text << "\n  unreachable\n";
        start_block(success_label);
        return;
    }
    if (recoverable_errors_)
    {
        track_pointer_instruction(text);
    }
    body_ << "  " << text << '\n';
    if (recoverable_errors_ && text.find("call ") != std::string::npos &&
        text.find("@llvm.") == std::string::npos &&
        text.find("call i32 @txrt_") == std::string::npos)
    {
        // 无状态码的快速 ABI 和 TX 函数也必须在使用返回值前检查失败。
        emit_pending_error_check();
    }
}

void llvm_code_generator::emit_gc_safepoint()
{
    const auto status = temporary();
    write_instruction(status +
        " = call i32 @txrt_gc_safepoint_context(ptr %tx_context)");
    write_instruction("call void @txrt_require_success(i32 " + status + ")");
}

void llvm_code_generator::start_block(const std::string& name)
{
    body_ << name << ":\n";
    terminated_ = false;
    in_entry_block_ = false;
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

std::string llvm_code_generator::generate(const program& source, bool library_mode)
{
    gc_visiting_.clear();
    gc_neutral_cache_.clear();
    functions_.clear();
    structs_.clear();
    serde_schemas_.clear();
    serde_types_.clear();
    classes_.clear();
    module_.str({});
    module_.clear();
    globals_.str({});
    globals_.clear();
    next_string_ = 0;
    next_bind_ = 0;
    virtual_slot_count_ = source.virtual_slot_count;
    recoverable_errors_ = library_mode;
    for (const auto& function : source.functions)
    {
        functions_[function.name].push_back(&function);
        recoverable_errors_ |= contains_try(function.body);
        // 预期异常断言会从原生入口调用 TX 回调，回调必须通过状态交回错误。
        recoverable_errors_ |= function.external_name == "test.assert_throws" ||
            function.external_name == "test.assert_throws_code" ||
            function.external_name == "test.run_case";
    }
    for (const auto& definition : source.structs)
    {
        structs_.emplace(definition.name, &definition);
        for (const auto& method : definition.methods)
        {
            recoverable_errors_ |= contains_try(method.body);
            functions_[class_method_symbol(definition.name, method.name)]
                .push_back(&method);
        }
    }
    for (const auto& definition : source.classes)
    {
        classes_.emplace(definition.name, &definition);
        for (const auto& method : definition.methods)
        {
            recoverable_errors_ |= contains_try(method.body);
            functions_[class_method_symbol(definition.name, method.name)]
                .push_back(&method);
        }
    }
    module_ << "target triple = \"x86_64-w64-windows-gnu\"\n\n"
            << "%tx_runtime_context = type { ptr, i32 }\n"
            << "%tx_diagnostic_frame = type { ptr, ptr, i64, i64, ptr }\n"
            << "declare ptr @txrt_runtime_context()\n"
            << "declare i32 @txrt_prepare_console()\n"
            << "declare void @txrt_require_success(i32)\n"
            << "declare i32 @txrt_gc_safepoint_context(ptr)\n"
            << "declare i32 @txrt_print_i64(i64, i1)\n"
            << "declare i32 @txrt_print_f64(double, i1)\n"
            << "declare i32 @txrt_print_bool(i1, i1)\n"
            << "declare i32 @txrt_print_char(i8)\n"
            << "declare i32 @txrt_exit_code(i64)\n"
            << "declare i32 @txrt_float_to_int(double, ptr)\n"
            << "declare i32 @txrt_add_i64(i64, i64, ptr)\n"
            << "declare i32 @txrt_sub_i64(i64, i64, ptr)\n"
            << "declare i32 @txrt_mul_i64(i64, i64, ptr)\n"
            << "declare i32 @txrt_div_i64(i64, i64, ptr)\n"
            << "declare i32 @txrt_mod_i64(i64, i64, ptr)\n"
            << "declare i32 @txrt_neg_i64(i64, ptr)\n"
            << "declare i32 @txrt_div_f64(double, double, ptr)\n"
            << "declare { i64, i1 } @llvm.sadd.with.overflow.i64(i64, i64)\n"
            << "declare { i64, i1 } @llvm.ssub.with.overflow.i64(i64, i64)\n"
            << "declare { i64, i1 } @llvm.smul.with.overflow.i64(i64, i64)\n\n";
    write_external_declarations();
    module_ << "declare i32 @txrt_str_new(ptr, i64, ptr)\n"
            << "declare i32 @txrt_str_clone(ptr, ptr)\n"
            << "declare i1 @txrt_str_equals_literal(ptr, ptr, i64)\n"
            << "declare void @txrt_str_release(ptr)\n"
            << "declare i32 @txrt_str_concat(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_str_compare(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_str_len(ptr, ptr)\n"
            << "declare i32 @txrt_print_str(ptr, i1)\n"
            << "declare i32 @txrt_input(ptr, ptr)\n"
            << "declare i32 @txrt_input_or_none(ptr, ptr)\n"
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
            << "declare i1 @txrt_value_snapshot_scalar(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_value_snapshot_box(i8, i64, ptr, ptr)\n"
            << "declare i64 @txrt_value_snapshot_to_i64_fast(i8, i64, ptr)\n"
            << "declare double @txrt_value_snapshot_to_f64_fast(i8, i64, ptr)\n"
            << "declare i1 @txrt_value_snapshot_to_bool_fast(i8, i64, ptr)\n"
            << "declare i32 @txrt_value_deep_copy(ptr, ptr)\n"
            << "declare void @txrt_value_release(ptr)\n"
            << "declare i32 @txrt_value_assign(ptr, ptr)\n"
            << "declare i32 @txrt_value_set_i64(ptr, i64)\n"
            << "declare i32 @txrt_value_set_f64(ptr, double)\n"
            << "declare i32 @txrt_value_set_bool(ptr, i1)\n"
            << "declare i32 @txrt_value_to_i64(ptr, ptr)\n"
            << "declare i32 @txrt_value_to_f64(ptr, ptr)\n"
            << "declare i32 @txrt_value_to_bool(ptr, ptr)\n"
            << "declare i64 @txrt_value_to_i64_fast(ptr)\n"
            << "declare double @txrt_value_to_f64_fast(ptr)\n"
            << "declare i1 @txrt_value_to_bool_fast(ptr)\n"
            << "declare i32 @txrt_value_to_str(ptr, ptr)\n"
            << "declare i32 @txrt_value_is_none(ptr, ptr)\n"
            << "declare i32 @txrt_value_len(ptr, ptr)\n"
            << "declare i32 @txrt_value_print(ptr, i1)\n"
            << "declare i32 @txrt_value_require_type(ptr, ptr)\n"
            << "declare i32 @txrt_value_require_type_or_none(ptr, ptr)\n"
            << "declare i32 @txrt_closure_new(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_closure_bind(ptr, ptr, ptr, ptr, i64, ptr)\n"
            << "declare ptr @txrt_closure_code(ptr)\n"
            << "declare i32 @txrt_closure_parent(ptr, ptr)\n"
            << "declare i32 @txrt_closure_parent_borrow(ptr, ptr)\n"
            << "declare i32 @txrt_closure_capture(ptr, i64, ptr)\n"
            << "declare i32 @txrt_closure_capture_i64(ptr, i64, ptr)\n"
            << "declare i32 @txrt_closure_capture_f64(ptr, i64, ptr)\n"
            << "declare i32 @txrt_closure_capture_bool(ptr, i64, ptr)\n"
            << "declare i32 @txrt_array_new(i64, ptr)\n"
            << "declare ptr @txrt_array_ref(ptr)\n"
            << "declare void @txrt_array_index_error()\n"
            << "declare i32 @txrt_local_scalar_array_new(i64, ptr)\n"
            << "declare void @txrt_local_scalar_array_release(ptr)\n"
            << "declare i64 @txrt_array_ref_len(ptr)\n"
            << "declare i64 @txrt_array_ref_get_i64(ptr, i64)\n"
            << "declare double @txrt_array_ref_get_f64(ptr, i64)\n"
            << "declare ptr @txrt_array_ref_get_str(ptr, i64)\n"
            << "declare ptr @txrt_array_ref_element_read_ptr(ptr, i64)\n"
            << "declare ptr @txrt_array_ref_element_ptr(ptr, i64)\n"
            << "declare i32 @txrt_array_ref_set_i64(ptr, i64, i64)\n"
            << "declare i32 @txrt_array_ref_set_f64(ptr, i64, double)\n"
            << "declare i32 @txrt_array_ref_set_bool(ptr, i64, i1)\n"
            << "declare i32 @txrt_array_ref_set_str(ptr, i64, ptr)\n"
            << "declare i32 @txrt_array_resize(i64, ptr, ptr)\n"
            << "declare i32 @txrt_array_len(ptr, ptr)\n"
            << "declare i32 @txrt_array_element_address(ptr, i64, ptr)\n"
            << "declare ptr @txrt_array_element_ptr(ptr, i64)\n"
            << "declare ptr @txrt_array_element_read_ptr(ptr, i64)\n"
            << "declare i32 @txrt_array_set_i64(ptr, i64, i64)\n"
            << "declare i32 @txrt_array_set_f64(ptr, i64, double)\n"
            << "declare i32 @txrt_array_set_bool(ptr, i64, i1)\n"
            << "declare i32 @txrt_array_set_str(ptr, i64, ptr)\n"
            << "declare i32 @txrt_array_set_value(ptr, i64, ptr)\n"
            << "declare i32 @txrt_array_append(ptr, ptr)\n"
            << "declare i32 @txrt_array_extend(ptr, ptr)\n"
            << "declare void @txrt_array_require_spread(ptr)\n"
            << "declare i32 @txrt_array_require_length(ptr, i64)\n"
            << "declare i32 @txrt_dict_new(ptr)\n"
            << "declare ptr @txrt_dict_ref(ptr)\n"
            << "declare i64 @txrt_dict_ref_len(ptr)\n"
            << "declare ptr @txrt_dict_ref_element_read_ptr(ptr, ptr)\n"
            << "declare ptr @txrt_dict_ref_element_read_ptr_str(ptr, ptr)\n"
            << "declare ptr @txrt_dict_ref_element_read_ptr_literal(ptr, ptr, i64)\n"
            << "declare i64 @txrt_dict_ref_get_i64(ptr, ptr)\n"
            << "declare i64 @txrt_dict_ref_get_i64_str(ptr, ptr)\n"
            << "declare i64 @txrt_dict_ref_get_i64_literal(ptr, ptr, i64)\n"
            << "declare double @txrt_dict_ref_get_f64(ptr, ptr)\n"
            << "declare double @txrt_dict_ref_get_f64_str(ptr, ptr)\n"
            << "declare double @txrt_dict_ref_get_f64_literal(ptr, ptr, i64)\n"
            << "declare ptr @txrt_dict_ref_get_str(ptr, ptr)\n"
            << "declare ptr @txrt_dict_ref_get_str_str(ptr, ptr)\n"
            << "declare ptr @txrt_dict_ref_get_str_literal(ptr, ptr, i64)\n"
            << "declare i32 @txrt_dict_set(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_dict_set_i64_str(ptr, ptr, i64)\n"
            << "declare i32 @txrt_dict_set_f64_str(ptr, ptr, double)\n"
            << "declare i32 @txrt_dict_set_bool_str(ptr, ptr, i1)\n"
            << "declare i32 @txrt_dict_set_str_str(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_dict_len(ptr, ptr)\n"
            << "declare i32 @txrt_dict_element_address(ptr, ptr, i1, ptr)\n"
            << "declare i32 @txrt_dict_element_address_str(ptr, ptr, i1, ptr)\n"
            << "declare i32 @txrt_dict_element_address_literal(ptr, ptr, i64, i1, ptr)\n"
            << "declare i32 @txrt_dictionary_get(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_dictionary_get_str(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_dictionary_contains(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_dictionary_contains_str(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_dictionary_remove(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_dictionary_remove_str(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_dictionary_keys(ptr, ptr)\n"
            << "declare i32 @txrt_dictionary_values(ptr, ptr)\n"
            << "declare i32 @txrt_dictionary_clear(ptr)\n"
            << "declare i32 @txrt_value_element_address(ptr, ptr, i1, ptr)\n"
            << "declare i32 @txrt_keyword_set(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_keyword_merge(ptr, ptr)\n"
            << "declare i32 @txrt_call_bind(ptr, ptr, ptr, i64, i32, i32, ptr)\n"
            << "declare i32 @txrt_call_split_spreads(ptr, ptr, ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_struct_new(ptr, ptr, i64, ptr)\n"
            << "declare i32 @txrt_struct_set_field(ptr, i64, ptr, ptr)\n"
            << "declare i32 @txrt_struct_set_field_i64(ptr, i64, ptr, i64)\n"
            << "declare i32 @txrt_struct_set_field_f64(ptr, i64, ptr, double)\n"
            << "declare i32 @txrt_struct_set_field_bool(ptr, i64, ptr, i1)\n"
            << "declare i32 @txrt_struct_field_address(ptr, ptr, ptr)\n"
            << "declare i32 @txrt_struct_field_address_index(ptr, i64, ptr)\n"
            << "declare ptr @txrt_struct_field_i64_ptr(ptr, i64)\n"
            << "declare ptr @txrt_struct_field_f64_ptr(ptr, i64)\n"
            << "declare ptr @txrt_struct_field_bool_ptr(ptr, i64)\n"
            << "declare i32 @txrt_class_new(ptr, ptr, ptr, i64, ptr, i64, ptr, i64, ptr, i64, ptr)\n"
            << "declare i32 @txrt_class_field_address_index(ptr, i64, i1, ptr)\n"
            << "declare ptr @txrt_class_field_i64_ptr(ptr, i64)\n"
            << "declare ptr @txrt_class_field_f64_ptr(ptr, i64)\n"
            << "declare ptr @txrt_class_field_bool_ptr(ptr, i64)\n"
            << "declare i32 @txrt_class_virtual_target(ptr, i64, ptr)\n"
            << "declare ptr @txrt_class_virtual_target_fast(ptr, i64)\n"
            << "declare i32 @txrt_class_require_type(ptr, ptr)\n"
            << "declare void @txrt_class_require_type_fast(ptr, ptr)\n"
            << "\n";

    for (const auto& definition : source.classes)
    {
        emit_class_metadata(definition);
    }

    for (const auto& function : source.functions)
    {
        emit_function(function);
    }
    for (const auto& definition : source.structs)
    {
        for (const auto& method : definition.methods)
        {
            emit_function(method);
        }
        emit_key_callbacks(definition);
    }
    for (const auto& definition : source.classes)
    {
        for (const auto& method : definition.methods)
        {
            emit_function(method);
        }
    }
    if (!library_mode)
    {
    module_ << "define i32 @main() {\nentry:\n"
            << "  %prepare = call i32 @txrt_prepare_console()\n"
            << "  call void @txrt_require_success(i32 %prepare)\n"
            << "  %system = call i32 @txrt_system_initialize()\n"
            << "  call void @txrt_require_success(i32 %system)\n";
    if (recoverable_errors_)
    {
        module_ << "  call void @txrt_error_propagation(i1 true)\n";
    }
    write_context_boundary();
    module_ << "  %result = call i64 " << function_name("main", 0)
            << "(ptr %tx_context)\n";
    if (recoverable_errors_)
    {
        module_ << "  call void @txrt_error_propagation(i1 false)\n"
                << "  %error = load i32, ptr %tx_error_kind\n"
                << "  call void @txrt_require_success(i32 %error)\n";
    }
    module_
            << "  %exit = call i32 @txrt_exit_code(i64 %result)\n"
            << "  ret i32 %exit\n}\n";
    }
    module_ << globals_.str();
    return module_.str();
}

} // namespace tx
