#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::store_variable(const variable_slot& variable,
                                          const ir_value& value, source_pos position)
{
    ir_value previous{value_type::void_type, {}};
    if (recoverable_errors_ &&
        (variable.type == value_type::str_type || is_value_handle(variable.type)))
    {
        const auto held = temporary();
        const auto old_root = allocate(variable.type, position);
        write_instruction(held + " = load ptr, ptr " + variable.address);
        write_instruction("store ptr " + held + ", ptr " + old_root);
        previous = {variable.type, held};
    }
    else
    {
        release_slot(variable);
    }
    // 先提交新值和缓存，再析构旧值；析构失败也不能让外层变量留下悬空句柄。
    write_instruction("store " + llvm_type(variable.type, position) + " " +
                      value.text + ", ptr " + variable.address);
    refresh_array_reference(variable, value.text);
    refresh_dict_reference(variable, value.text);
    release(previous);
}

void llvm_code_generator::forget_owned_value(const ir_value& value)
{
    if (!recoverable_errors_)
    {
        return;
    }
    const auto found = pointer_owners_.find(value.text);
    if (found != pointer_owners_.end())
    {
        body_ << "  store ptr null, ptr " << found->second << '\n';
    }
}

llvm_code_generator::ir_value llvm_code_generator::own_direct_value(ir_value value)
{
    if (recoverable_errors_ &&
        (value.type == value_type::str_type || is_value_handle(value.type)) &&
        !pointer_owners_.contains(value.text))
    {
        const auto root = allocate(value.type, {});
        body_ << "  store ptr " << value.text << ", ptr " << root << '\n';
        pointer_owners_[value.text] = root;
    }
    return value;
}

void llvm_code_generator::track_pointer_instruction(const std::string& text)
{
    constexpr std::string_view load_marker = " = load ptr, ptr ";
    if (const auto at = text.find(load_marker); at != std::string::npos)
    {
        const auto address = text.substr(at + load_marker.size());
        if (error_root_indices_.contains(address))
        {
            pointer_owners_[text.substr(0, at)] = address;
        }
        return;
    }
    constexpr std::string_view store_prefix = "store ptr ";
    if (text.starts_with(store_prefix))
    {
        const auto split = text.find(", ptr ", store_prefix.size());
        if (split == std::string::npos)
        {
            return;
        }
        const auto value = text.substr(store_prefix.size(), split - store_prefix.size());
        const auto destination = text.substr(split + 6);
        const auto source = pointer_owners_.find(value);
        if (source != pointer_owners_.end() &&
            error_root_indices_.contains(destination) && source->second != destination)
        {
            // 只追踪拥有句柄的槽；字段地址和借用缓存不参与所有权转移。
            body_ << "  store ptr null, ptr " << source->second << '\n';
            source->second = destination;
        }
        else if (error_root_indices_.contains(destination) && value != "null")
        {
            pointer_owners_[value] = destination;
        }
        return;
    }
    for (const std::string_view prefix : {
             "call void @txrt_str_release(ptr ", "call void @txrt_value_release(ptr "})
    {
        if (text.starts_with(prefix) && text.ends_with(')'))
        {
            forget_owned_value({value_type::any_type,
                text.substr(prefix.size(), text.size() - prefix.size() - 1)});
            return;
        }
    }
    if (text.starts_with("ret ptr "))
    {
        forget_owned_value({return_type_, text.substr(8)});
    }
}

void llvm_code_generator::transfer_call_arguments(
    const function_decl& target, const std::vector<ir_value>& arguments)
{
    const std::size_t offset = target.owner_class.empty() ? 0 : 1;
    for (std::size_t index = 0; index < target.parameters.size(); ++index)
    {
        if (!ordinary_parameter_borrowed(target, index) &&
            !init_parameter_borrowed(target, index) &&
            !(index == 0 && operator_parameter_borrowed(target)))
        {
            forget_owned_value(arguments[index + offset]);
        }
    }
}

void llvm_code_generator::emit_error_cleanup(std::size_t minimum_depth)
{
    for (auto root = error_roots_.rbegin(); root != error_roots_.rend(); ++root)
    {
        if (root->depth < minimum_depth)
        {
            continue;
        }
        const auto value = temporary();
        // 先清槽再析构，避免析构再次失败时重复释放。同一槽在循环中可重复使用。
        body_ << "  " << value << " = load ptr, ptr " << root->address << '\n'
              << "  store ptr null, ptr " << root->address << '\n'
              << "  call void @"
              << (root->type == value_type::str_type ? "txrt_str_release" : "txrt_value_release")
              << "(ptr " << value << ")\n";
    }
}

} // namespace tx
