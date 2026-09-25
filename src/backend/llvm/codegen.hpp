#pragma once

#include "frontend/ast/ast.hpp"

#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tx
{

class llvm_code_generator
{
public:
    [[nodiscard]] std::string generate(const program& source);

private:
    struct ir_value
    {
        value_type type;
        std::string text;
    };

    struct variable_slot
    {
        value_type type;
        std::string address;
        bool borrowed = false;
        std::string array_reference;
        std::string snapshot_kind;
        std::string snapshot_bits;
        std::optional<std::size_t> local_array_length;
        std::string dynamic_array_length;
        std::string dict_reference;

        variable_slot(value_type value_type, std::string value_address,
                      bool is_borrowed = false,
                      std::string native_array = {},
                      std::string scalar_kind = {},
                      std::string scalar_bits = {},
                      std::optional<std::size_t> local_length = std::nullopt,
                      std::string runtime_length = {})
            : type(std::move(value_type)), address(std::move(value_address)),
              borrowed(is_borrowed),
              array_reference(std::move(native_array)),
              snapshot_kind(std::move(scalar_kind)),
              snapshot_bits(std::move(scalar_bits)),
              local_array_length(local_length),
              dynamic_array_length(std::move(runtime_length))
        {
        }
    };

    [[nodiscard]] static std::string llvm_type(const value_type& type,
                                               source_pos position);
    [[nodiscard]] static bool is_value_handle(const value_type& type);
    [[nodiscard]] static std::string function_name(const std::string& name,
                                                   std::size_t overload);
    [[nodiscard]] std::string temporary();
    [[nodiscard]] std::string label();
    [[nodiscard]] std::string random_context();
    [[nodiscard]] std::string allocate(const value_type& type,
                                       source_pos position);
    [[nodiscard]] std::string global_bytes(std::string_view bytes);
    [[nodiscard]] static std::string decode_string_literal(
        std::string_view quoted);
    [[nodiscard]] variable_slot find_variable(const std::string& name,
                                              source_pos position) const;
    [[nodiscard]] std::size_t field_index(const value_type& type,
                                          std::string_view field,
                                          source_pos position) const;
    [[nodiscard]] std::string scalar_field_address(
        const expression& item);
    [[nodiscard]] bool direct_scalar_field(
        const expression& item) const;
    [[nodiscard]] ir_value load(const variable_slot& variable);
    [[nodiscard]] std::string cache_array_reference(
        const std::string& handle, source_pos position);
    void refresh_array_reference(const variable_slot& variable,
                                 const std::string& handle);
    [[nodiscard]] std::string load_array_reference(
        const variable_slot& variable);
    [[nodiscard]] std::string cache_dict_reference(
        const std::string& handle, source_pos position);
    void refresh_dict_reference(const variable_slot& variable,
                                const std::string& handle);
    [[nodiscard]] std::string load_dict_reference(
        const variable_slot& variable);
    [[nodiscard]] variable_slot make_scalar_snapshot(
        const std::string& element, source_pos position);
    [[nodiscard]] ir_value cast_scalar_snapshot(
        const variable_slot& variable, const value_type& target,
        source_pos position);
    [[nodiscard]] std::optional<std::size_t> eligible_local_array_length(
        const variable_declaration& declaration) const;
    [[nodiscard]] bool eligible_dynamic_local_array(
        const variable_declaration& declaration) const;
    [[nodiscard]] static bool has_local_array(const variable_slot& array);
    void emit_dynamic_local_array_declaration(
        const statement& item, const variable_declaration& declaration);
    void emit_local_array_declaration(const statement& item,
                                      const variable_declaration& declaration,
                                      std::size_t length);
    [[nodiscard]] bool emit_local_array_assignment(
        const statement& item, const variable_assignment& assignment);
    void emit_local_array_for_each(const for_each& loop,
                                   const variable_slot& array);
    void emit_local_array_unpack(const statement& item,
                                 const unpack_assignment& assignment,
                                 const variable_slot& array);
    [[nodiscard]] std::pair<std::string, std::string> local_array_element(
        const variable_slot& array, const std::string& index,
        bool known_valid);
    void store_local_scalar(const std::string& kind_address,
                            const std::string& bits_address,
                            const ir_value& value);
    [[nodiscard]] variable_slot make_local_scalar_snapshot(
        const std::string& kind_address,
        const std::string& bits_address, source_pos position);
    [[nodiscard]] ir_value cast_local_array_element(
        const index_expression& index, const value_type& target,
        source_pos position);
    void release(const ir_value& value);
    void release_slot(const variable_slot& variable);
    [[nodiscard]] ir_value expression_value(const expression& item);
    [[nodiscard]] static std::string vector_suffix(const value_type& type);
    [[nodiscard]] static bool stable_value_expression(const expression& item);
    [[nodiscard]] ir_value container_value(const expression& item, bool allow_borrow,
                                         bool& borrowed);
    [[nodiscard]] std::string vector_slot(const ir_value& value, const ir_value& index,
                                          source_pos position);
    [[nodiscard]] ir_value vector_read(const ir_value& value, const ir_value& index,
                                       source_pos position);
    void vector_write(const ir_value& value, const ir_value& index,
                      const ir_value& element, source_pos position);
    [[nodiscard]] ir_value vector_length(const ir_value& value, bool capacity);
    [[nodiscard]] ir_value emit_vector_index(const index_expression& access,
                                             source_pos position);
    [[nodiscard]] ir_value emit_vector_call(const expression& item,
                                            const call_expression& call);
    void emit_vector_assignment(const statement& item,
                                const variable_assignment& assignment);
    [[nodiscard]] ir_value emit_vector_update(const expression& item,
                                              const update_expression& update);
    void emit_vector_for_each(const for_each& loop);
    void write_vector_declarations();
    void write_container_declarations();
    void write_algorithm_declarations();
    [[nodiscard]] static std::string container_symbol(const value_type& type,
                                                      std::string_view operation);
    [[nodiscard]] ir_value container_operation(const value_type& type,
        std::string_view operation, const std::vector<ir_value>& arguments,
        const value_type& result_type, source_pos position);
    [[nodiscard]] ir_value emit_container_call(const expression& item,
                                               const call_expression& call);
    [[nodiscard]] ir_value emit_map_index(const index_expression& access,
                                          source_pos position);
    void emit_map_assignment(const statement& item, const variable_assignment& assignment);
    [[nodiscard]] ir_value emit_map_update(const expression& item,
                                           const update_expression& update);
    [[nodiscard]] ir_value map_update_value(const ir_value& current, const ir_value& value,
                                            bool add, source_pos position);
    void initialize_container_fields(const class_decl& definition,
                                      const std::string& object, source_pos position);
    void emit_array_elements(const array_literal& literal,
                             const std::string& array, source_pos position);
    [[nodiscard]] ir_value expression_value_or_borrow(
        const expression& item, bool& borrowed);
    [[nodiscard]] ir_value emit_binary(const expression& item,
                                       const binary_operation& operation);
    [[nodiscard]] std::optional<ir_value> emit_literal_string_compare(
        const binary_operation& operation);
    [[nodiscard]] std::optional<ir_value> emit_string_concat(
        const expression& item, const binary_operation& operation);
    [[nodiscard]] ir_value read_only_string_value(const expression& item, bool& borrowed);
    [[nodiscard]] std::optional<ir_value> emit_string_key_query(
        const expression& item, const call_expression& call, const function_decl& target);
    [[nodiscard]] ir_value emit_operator_call(
        const value_type& result_type, source_pos position,
        const operator_binding& binding, const ir_value& receiver,
        const std::optional<ir_value>& argument,
        bool receiver_borrowed = false, bool argument_borrowed = false);
    [[nodiscard]] ir_value emit_call(const expression& item,
                                     const call_expression& call);
    [[nodiscard]] ir_value emit_builtin_call(
        const expression& item, const call_expression& call,
        const std::vector<ir_value>& arguments, bool borrowed_argument = false);
    void emit_print_call(const expression& item, const call_expression& call,
                         const std::vector<ir_value>& arguments);
    void emit_print_value(const ir_value& value, bool newline,
                          source_pos position);
    void emit_print_char(int value);
    [[nodiscard]] ir_value emit_constructor_call(
        const expression& item, const call_expression& call,
        const std::vector<ir_value>& arguments);
    [[nodiscard]] ir_value emit_class_constructor(
        const expression& item, const call_expression& call,
        const std::vector<ir_value>& arguments,
        const std::vector<bool>& borrowed_arguments);
    [[nodiscard]] ir_value emit_method_call(
        const expression& item, const call_expression& call);
    void emit_class_metadata(const class_decl& definition);
    void collect_class_nodes(const class_decl& definition,
                             std::vector<const class_decl*>& result) const;
    [[nodiscard]] std::size_t class_field_count(
        const class_decl& definition) const;
    [[nodiscard]] bool class_is_assignable(const value_type& actual,
                                           const value_type& expected) const;
    [[nodiscard]] ir_value emit_external_call(
        const expression& item, const call_expression& call,
        const function_decl& target, const std::vector<ir_value>& arguments);
    [[nodiscard]] ir_value emit_direct_external_call(
        const expression& item, const function_decl& target,
        const std::vector<ir_value>& arguments);
    [[nodiscard]] std::optional<ir_value> emit_literal_string_call(
        const expression& item, const call_expression& call,
        const function_decl& target);
    [[nodiscard]] ir_value emit_user_call(
        const expression& item, const call_expression& call,
        const function_decl& target, const std::vector<ir_value>& arguments);
    [[nodiscard]] ir_value emit_cast(const expression& item,
                                     const cast_expression& cast);
    [[nodiscard]] ir_value cast_array_element(const index_expression& index,
                                              const value_type& target,
                                              source_pos position);
    [[nodiscard]] ir_value cast_dict_element(const index_expression& index,
                                             const value_type& target,
                                             source_pos position);
    [[nodiscard]] ir_value checked_binary(const std::string& name,
                                          const ir_value& left,
                                          const ir_value& right,
                                          const value_type& result_type,
                                          source_pos position);
    [[nodiscard]] ir_value checked_unary(const std::string& name,
                                         const ir_value& value,
                                         const value_type& result_type,
                                         source_pos position);
    [[nodiscard]] ir_value short_circuit(const binary_operation& operation);
    [[nodiscard]] ir_value box_any(const ir_value& value, source_pos position);
    [[nodiscard]] ir_value from_any(const ir_value& value,
                                    const value_type& target,
                                    source_pos position);
    void assign_any(const std::string& target, const ir_value& value,
                    source_pos position);
    using lvalue_indices = std::unordered_map<const expression*, ir_value>;
    void prepare_lvalue_indices(const expression& item, lvalue_indices& indices);
    [[nodiscard]] std::string lvalue_address(
        const expression& item, const lvalue_indices& indices);
    [[nodiscard]] ir_value emit_update(const expression& item,
                                       const update_expression& operation);
    void emit_statement(const statement& item);
    [[nodiscard]] bool gc_neutral_expression(const expression& item);
    [[nodiscard]] bool gc_neutral_statement(const statement& item);
    [[nodiscard]] bool gc_neutral_body(const std::vector<stmt_ptr>& body);
    [[nodiscard]] bool gc_neutral_function(const function_decl& function);
    [[nodiscard]] bool gc_neutral_binding(const operator_binding& binding);
    [[nodiscard]] static bool operator_parameter_intrinsic(
        const function_decl& function);
    [[nodiscard]] static bool init_parameter_borrowed(
        const function_decl& function, std::size_t index);
    [[nodiscard]] static bool ordinary_parameter_borrowed(
        const function_decl& function, std::size_t index);
    [[nodiscard]] static bool rebinds_name(const std::vector<stmt_ptr>& body,
                                           std::string_view name);
    [[nodiscard]] bool operator_parameter_borrowed(
        const function_decl& function) const;
    [[nodiscard]] bool operator_argument_borrowed(
        const operator_binding& binding) const;
    void emit_declaration(const statement& item,
                          const variable_declaration& declaration);
    void emit_assignment(const statement& item,
                         const variable_assignment& assignment);
    void emit_unpack(const statement& item,
                     const unpack_assignment& assignment);
    [[nodiscard]] std::vector<ir_value> emit_bound_arguments(
        const expression& item, const call_expression& call,
        const std::vector<parameter>& parameters);
    [[nodiscard]] std::optional<std::vector<ir_value>> emit_direct_spreads(
        const expression& item, const call_expression& call,
        const std::vector<parameter>& parameters);
    [[nodiscard]] std::vector<ir_value> emit_static_arguments(
        const expression& item, const call_expression& call,
        const std::vector<parameter>& parameters);
    [[nodiscard]] ir_value emit_variadic_array(
        const std::vector<ir_value>& values, source_pos position);
    [[nodiscard]] ir_value emit_variadic_dict(
        const std::vector<std::pair<std::string, ir_value>>& values,
        source_pos position);
    void emit_name_assignment(const statement& item,
                              const variable_assignment& assignment,
                              const name_reference& name);
    void emit_composite_assignment(const statement& item,
                                   const variable_assignment& assignment);
    [[nodiscard]] bool emit_direct_array_assignment(
        const statement& item, const variable_assignment& assignment);
    [[nodiscard]] bool emit_array_copy_assignment(
        const statement& item, const variable_assignment& assignment);
    [[nodiscard]] bool emit_direct_dict_assignment(
        const statement& item, const variable_assignment& assignment);
    void emit_return(const statement& item, const return_statement& result);
    void emit_statements(const std::vector<stmt_ptr>& statements);
    void emit_if(const if_statement& branch);
    void emit_while(const while_statement& loop);
    void emit_for(const for_loop& loop);
    void emit_for_each(const for_each& loop);
    void emit_function(const function_decl& function);
    void write_external_declarations();
    void write_instruction(const std::string& text);
    void emit_gc_safepoint();
    void start_block(const std::string& name);
    void push_scope();
    void pop_scope();

    std::unordered_map<std::string, std::vector<const function_decl*>> functions_;
    std::unordered_map<std::string, const struct_decl*> structs_;
    std::unordered_map<std::string, const class_decl*> classes_;
    std::vector<std::unordered_map<std::string, variable_slot>> scopes_;
    std::ostringstream module_;
    std::ostringstream globals_;
    std::ostringstream allocations_;
    std::ostringstream body_;
    value_type return_type_ = value_type::void_type;
    std::size_t next_value_ = 0;
    std::size_t next_slot_ = 0;
    std::size_t next_label_ = 0;
    std::size_t next_string_ = 0;
    std::string random_context_slot_;
    std::string current_method_owner_;
    std::unordered_map<std::size_t, std::string> entry_scalar_field_cache_;
    std::size_t virtual_slot_count_ = 0;
    bool terminated_ = false;
    bool in_entry_block_ = true;
    std::unordered_set<const function_decl*> gc_visiting_;
    std::unordered_map<const function_decl*, bool> gc_neutral_cache_;
    const std::vector<stmt_ptr>* current_function_body_ = nullptr;
};

} // namespace tx
