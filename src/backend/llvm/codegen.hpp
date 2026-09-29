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

struct static_format_part;

class llvm_code_generator
{
public:
    [[nodiscard]] std::string generate(const program& source,
                                       bool library_mode = false);

private:
    using integer_interval = std::pair<std::int64_t, std::int64_t>;
    struct ir_value
    {
        value_type type;
        std::string text;
        bool borrowed = false;
        std::string vector_reference = {};
        std::string vector_data = {};
        std::string vector_size = {};
        std::optional<integer_interval> integer_range = std::nullopt;
        std::string record_view = {};
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
        std::string native_option_value;
        std::string native_parse_ok;
        std::string native_parse_value;
        std::string native_parse_error;
        std::string vector_reference;
        std::optional<std::string> constant_text;
        bool default_heap = false;
        std::string closure_view;
        std::string closure_target;
        const function_decl* closure_function = nullptr;
        std::string record_view;
        std::string stack_record;
        std::string iterator_cursor;
        std::optional<integer_interval> integer_range;
        const variable_declaration* parse_declaration = nullptr;
        std::string readonly_parse_error;
        bool stable_class_owner = false;

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
    [[nodiscard]] static value_type parameter_abi_type(const parameter& value);
    [[nodiscard]] static bool is_value_handle(const value_type& type);
    [[nodiscard]] static std::string function_name(const std::string& name,
                                                   std::size_t overload);
    [[nodiscard]] static std::string callback_name(const std::string& name,
                                                   std::size_t overload);
    [[nodiscard]] std::string temporary();
    [[nodiscard]] std::string label();
    [[nodiscard]] std::string allocate(const value_type& type,
                                       source_pos position, bool owned = true);
    struct error_root
    {
        value_type type;
        std::string address;
        std::size_t depth;
    };
    struct error_target
    {
        std::string label;
        std::size_t depth;
    };
    void emit_try(const try_statement& guarded);
    void emit_error_check(const std::string& status);
    void emit_pending_error_check();
    void emit_error_exit();
    void emit_error_cleanup(std::size_t minimum_depth);
    void track_pointer_instruction(const std::string& text);
    void forget_owned_value(const ir_value& value);
    [[nodiscard]] ir_value own_direct_value(ir_value value);
    void transfer_call_arguments(const function_decl& target,
                                  const std::vector<ir_value>& arguments);
    [[nodiscard]] static bool contains_try(const std::vector<stmt_ptr>& body);
    [[nodiscard]] std::string global_bytes(std::string_view bytes);
    [[nodiscard]] static std::string decode_string_literal(
        std::string_view quoted);
    [[nodiscard]] variable_slot find_variable(const std::string& name,
                                              source_pos position) const;
    [[nodiscard]] std::size_t field_index(const value_type& type,
                                          std::string_view field,
                                          source_pos position) const;
    [[nodiscard]] std::string scalar_field_address(
        const expression& item, bool read_only = false);
    [[nodiscard]] bool confined_local(const variable_declaration& declaration,
                                     bool fields_only) const;
    [[nodiscard]] bool readonly_local_fields(const variable_declaration& declaration,
        const variable_declaration* allowed_alias = nullptr) const;
    [[nodiscard]] bool emit_parse_error_declaration(const statement& item,
        const variable_declaration& declaration);
    [[nodiscard]] std::optional<ir_value> emit_parse_error_field(const expression& item,
        bool length = false);
    [[nodiscard]] bool default_heap_local(const variable_declaration& declaration) const;
    [[nodiscard]] bool default_heap_receiver(const call_expression& call) const;
    [[nodiscard]] bool direct_scalar_field(
        const expression& item) const;
    [[nodiscard]] bool static_record_type(const value_type& type) const;
    void emit_record_metadata(const value_type& type);
    void cache_record_view(variable_slot& slot, const std::string& value);
    void refresh_record_view(const variable_slot& slot, const std::string& value);
    [[nodiscard]] std::string record_view_value(const ir_value& object);
    [[nodiscard]] std::string load_record_view(const variable_slot& variable);
    [[nodiscard]] std::string record_virtual_target(const std::string& view, std::size_t slot);
    [[nodiscard]] std::string record_field_slot(const ir_value& object, std::size_t index);
    [[nodiscard]] ir_value read_record_field(const ir_value& object,
        const expression& item, const member_expression& member);
    [[nodiscard]] std::optional<ir_value> emit_record_string_length(const expression& item);
    [[nodiscard]] bool emit_borrowed_class_cast(const statement& item,
                                               const variable_declaration& declaration);
    [[nodiscard]] bool borrowed_class_local(const variable_declaration& declaration) const;
    [[nodiscard]] bool emit_stack_record(const statement& item,
                                         const variable_declaration& declaration);
    [[nodiscard]] bool scalar_record_type(const value_type& type) const;
    [[nodiscard]] bool native_record_function(const function_decl& function) const;
    [[nodiscard]] const function_decl* native_record_target(const call_expression& call) const;
    [[nodiscard]] const function_decl* native_record_target(const operator_binding& binding) const;
    [[nodiscard]] bool native_record_expression(const expression& item) const;
    [[nodiscard]] bool native_record_gc_neutral(const expression& item);
    [[nodiscard]] std::string native_record_data(const expression& item,
                                                std::vector<ir_value>& owned);
    [[nodiscard]] std::string allocate_record_data(const value_type& type);
    void copy_record_data(const value_type& type, const std::string& source,
                          const std::string& target);
    [[nodiscard]] ir_value native_record_call(const function_decl& function,
        const expression* receiver, const std::vector<const expression*>& arguments,
        source_pos position);
    [[nodiscard]] bool emit_native_record_assignment(const variable_assignment& assignment);
    [[nodiscard]] bool emit_record_assignment(const statement& item,
                                             const variable_assignment& assignment);
    [[nodiscard]] ir_value emit_record_constructor(const expression& item,
        const std::vector<ir_value>& arguments);
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
    void store_variable(const variable_slot& variable, const ir_value& value,
                         source_pos position);
    [[nodiscard]] ir_value expression_value(const expression& item);
    [[nodiscard]] static std::string vector_suffix(const value_type& type);
    [[nodiscard]] std::string vector_reference(const ir_value& value);
    void cache_vector_reference(variable_slot& variable, const std::string& handle);
    void refresh_vector_reference(const variable_slot& variable, const std::string& handle);
    [[nodiscard]] std::string load_vector_reference(const variable_slot& variable);
    [[nodiscard]] static bool stable_vector_loop(const std::vector<stmt_ptr>& body);
    [[nodiscard]] static bool stable_value_expression(const expression& item);
    [[nodiscard]] ir_value container_value(const expression& item, bool allow_borrow,
                                         bool& borrowed);
    [[nodiscard]] std::string vector_slot(const ir_value& value, const ir_value& index,
                                          source_pos position, bool known_valid = false);
    [[nodiscard]] ir_value vector_read(const ir_value& value, const ir_value& index,
                                       source_pos position, bool known_valid = false);
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
    void write_sum_declarations();
    void write_iterator_declarations();
    [[nodiscard]] static const char* scalar_option_suffix(const value_type& type);
    [[nodiscard]] const call_expression* native_parse_initializer(
        const variable_declaration& declaration) const;
    [[nodiscard]] bool is_parse_result_type(const value_type& type) const;
    [[nodiscard]] bool gc_neutral_native_parse_declaration(
        const call_expression& call);
    [[nodiscard]] bool emit_native_parse_declaration(
        const statement& item, const variable_declaration& declaration);
    void materialize_native_parse(const variable_slot& variable);
    [[nodiscard]] std::string native_parse_field_address(
        const variable_slot& variable, const expression& item);
    [[nodiscard]] bool emit_native_option_declaration(
        const statement& item, const variable_declaration& declaration);
    [[nodiscard]] std::optional<ir_value> emit_native_option_method(
        const expression& item, const call_expression& call);
    [[nodiscard]] ir_value load_native_option(const variable_slot& variable);
    void store_native_option(const variable_slot& variable,
                             const ir_value& value, source_pos position);
    [[nodiscard]] ir_value emit_sum_call(const expression& item,
                                          const call_expression& call);
    [[nodiscard]] ir_value emit_iterator_call(const expression& item,
                                              const call_expression& call);
    [[nodiscard]] ir_value emit_bind_call(const expression& item,
                                          const call_expression& call);
    void emit_bind_wrapper(const std::string& name,
                           const value_type& parent_type,
                           std::size_t captured);
    void write_container_declarations();
    void write_ordered_declarations();
    void write_sequence_declarations();
    void emit_key_callbacks(const struct_decl& definition);
    [[nodiscard]] static std::string key_hash_symbol(const value_type& type);
    [[nodiscard]] static std::string key_equal_symbol(const value_type& type);
    [[nodiscard]] static std::string key_less_symbol(const value_type& type);
    void write_algorithm_declarations();
    [[nodiscard]] ir_value emit_algorithm_intrinsic(
        const expression& item, const function_decl& target,
        const std::vector<ir_value>& arguments);
    [[nodiscard]] ir_value emit_thread_intrinsic(
        const expression& item, const function_decl& target,
        const std::vector<ir_value>& arguments);
    [[nodiscard]] ir_value emit_task_intrinsic(
        const expression& item, const function_decl& target,
        const std::vector<ir_value>& arguments);
    [[nodiscard]] ir_value emit_task_wait(
        const expression& item, const ir_value& argument);
    [[nodiscard]] ir_value emit_async_call(
        const expression& item, const call_expression& call,
        const function_decl& target);
    void write_sync_declarations();
    [[nodiscard]] ir_value emit_sync_intrinsic(
        const expression& item, const function_decl& target,
        const std::vector<ir_value>& arguments);
    void write_channel_declarations();
    [[nodiscard]] ir_value emit_channel_intrinsic(
        const expression& item, const function_decl& target,
        const std::vector<ir_value>& arguments);
    [[nodiscard]] ir_value emit_serde_intrinsic(
        const expression& item, const function_decl& target,
        const std::vector<ir_value>& arguments);
    [[nodiscard]] std::optional<ir_value> emit_static_format(
        const expression& item, const call_expression& call, const function_decl& target);
    [[nodiscard]] std::optional<ir_value> emit_dynamic_format(
        const expression& item, const call_expression& call, const function_decl& target);
    [[nodiscard]] std::optional<std::string> constant_format_text(const expression& item) const;
    [[nodiscard]] bool immutable_format_local(const variable_declaration& declaration) const;
    [[nodiscard]] std::vector<ir_value> emit_format_arguments(const call_expression& call,
        const std::vector<static_format_part>& plan, std::vector<std::optional<std::string>>& bytes);
    [[nodiscard]] std::optional<ir_value> emit_constant_encoding(
        const expression& item, const call_expression& call, const function_decl& target);
    [[nodiscard]] std::string serde_schema_constant(const value_type& type);
    [[nodiscard]] std::string serde_type_constant(const value_type& type);
    [[nodiscard]] std::string serde_default_constant(const struct_field& field);
    [[nodiscard]] std::string static_format_steps(const call_expression& call,
        const std::vector<struct static_format_part>& plan,
        const std::vector<std::optional<std::string>>& bytes);
    [[nodiscard]] std::string static_format_arguments(const std::vector<ir_value>& values,
        const std::vector<std::optional<std::string>>& bytes);
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
    [[nodiscard]] ir_value emit_precompiled_requests_method(
        const expression& item, const call_expression& call,
        const function_decl& target, const std::vector<ir_value>& arguments,
        bool borrowed_receiver);
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
    [[nodiscard]] ir_value call_argument_value(const call_expression& call,
                                               std::size_t index);
    [[nodiscard]] ir_value emit_direct_external_call(
        const expression& item, const function_decl& target,
        const std::vector<ir_value>& arguments);
    [[nodiscard]] std::optional<ir_value> emit_literal_string_call(
        const expression& item, const call_expression& call,
        const function_decl& target);
    [[nodiscard]] ir_value emit_user_call(
        const expression& item, const call_expression& call,
        const function_decl& target, const std::vector<ir_value>& arguments,
        std::string_view symbol_override = {}, std::string_view receiver_view = {});
    void emit_record_method_adapter(const function_decl& function,
                                     const std::string& symbol, std::size_t index);
    [[nodiscard]] std::vector<ir_value> coerce_nullable_arguments(
        const function_decl& target,
        const std::vector<ir_value>& arguments);
    [[nodiscard]] ir_value emit_callback_call(
        const expression& item, const call_expression& call);
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
    [[nodiscard]] bool gc_neutral_native_option_declaration(
        const variable_declaration& declaration);
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
    [[nodiscard]] static integer_interval integer_range_of(const ir_value& value);
    [[nodiscard]] std::uint64_t record_copy_tag(const value_type& type) const;
    [[nodiscard]] std::optional<ir_value> emit_proven_arithmetic(
        const std::string& name, const ir_value& left, const ir_value& right);
    [[nodiscard]] ir_value emit_division(const std::string& name,
        const ir_value& left, const ir_value& right, source_pos position);
    [[nodiscard]] static bool scalar_local_unchanged(const std::string& name,
        const std::vector<stmt_ptr>& body, const variable_declaration* allowed = nullptr);
    void refine_integer_condition(const expression& condition,
        const std::vector<stmt_ptr>& body, bool truth);
    void cache_iterator_cursor(variable_slot& slot,
        const variable_declaration& declaration, const std::string& handle);
    [[nodiscard]] bool emit_cursor_next(const call_expression& call,
        const std::string& present, const std::string& scalar);
    [[nodiscard]] std::vector<ir_value> emit_bound_arguments(
        const expression& item, const call_expression& call,
        const std::vector<parameter>& parameters);
    void emit_spread_defaults(const std::vector<parameter>& parameters,
                              std::size_t fixed_count,
                              const std::string& positional,
                              const std::string& keywords,
                              source_pos position);
    [[nodiscard]] std::optional<std::vector<ir_value>> emit_direct_spreads(
        const expression& item, const call_expression& call,
        const std::vector<parameter>& parameters);
    [[nodiscard]] std::optional<std::vector<ir_value>> emit_literal_spreads(
        const expression& item, const call_expression& call,
        const std::vector<parameter>& parameters);
    [[nodiscard]] std::string call_parameter_names(const std::vector<parameter>& parameters,
        std::size_t fixed_count);
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
    void emit_function(const function_decl& function, bool native = false);
    [[nodiscard]] std::optional<integer_interval> constant_integer_result(const function_decl& function) const;
    [[nodiscard]] std::optional<integer_interval> bounded_integer_result(const function_decl& function);
    const function_decl* proven_integer_function_ = nullptr;
    bool generating_bounded_integer_ = false;
    std::unordered_map<const function_decl*, std::optional<integer_interval>> bounded_integer_results_;
    std::unordered_set<const expression*> emitted_bounded_calls_;
    void emit_callback_wrapper(const function_decl& function,
                               const std::string& symbol,
                               std::size_t overload);
    void write_nullable_callback_boxes(const function_decl& function);
    void emit_native_closure_adapter(const std::string& name,
                                     const value_type& type, bool bound);
    void emit_typed_bind_wrapper(const std::string& name,
                                 const value_type& parent_type, std::size_t captured);
    void cache_closure_local(variable_slot& slot, const variable_declaration& declaration,
                             const std::string& value);
    void write_callback_releases(const function_decl& function,
                                 bool call_completed);
    void write_external_declarations();
    void write_instruction(const std::string& text);
    void write_context_boundary();
    void write_stack_frame(const function_decl& function);
    void emit_stack_pop();
    void emit_stack_location();
    std::optional<source_pos> last_stack_position_;
    void emit_error_location();
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
    std::unordered_map<std::string, std::string> serde_schemas_;
    std::unordered_map<std::string, std::string> serde_types_;
    std::ostringstream allocations_;
    std::ostringstream body_;
    value_type return_type_ = value_type::void_type;
    value_type native_result_type_ = value_type::void_type;
    std::size_t next_value_ = 0;
    std::size_t next_slot_ = 0;
    std::size_t next_label_ = 0;
    std::size_t next_string_ = 0;
    std::size_t next_bind_ = 0;
    std::string current_method_owner_;
    std::unordered_map<std::size_t, std::string> entry_scalar_field_cache_;
    std::size_t virtual_slot_count_ = 0;
    bool terminated_ = false;
    bool in_entry_block_ = true;
    std::unordered_set<const function_decl*> gc_visiting_;
    std::unordered_map<const function_decl*, bool> gc_neutral_cache_;
    const std::vector<stmt_ptr>* current_function_body_ = nullptr;
    const source_pos* current_statement_position_ = nullptr;
    bool recoverable_errors_ = false;
    bool library_mode_ = false;
    std::vector<error_root> error_roots_;
    std::unordered_map<std::string, std::size_t> error_root_indices_;
    std::unordered_map<std::string, std::string> pointer_owners_;
    std::vector<error_target> error_targets_;
};

} // namespace tx
