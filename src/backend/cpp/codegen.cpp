#include "backend/cpp/codegen.hpp"

#include "backend/cpp/runtime_source.hpp"

#include <string>

namespace tx
{

void code_generator::write_line(std::string_view text)
{
    output_ << std::string(indent_ * 4, ' ') << text << '\n';
}

void code_generator::emit_for(const for_loop& loop)
{
    const auto suffix = std::to_string(loop_index_++);
    const auto first = "tx_start_" + suffix;
    const auto last = "tx_end_" + suffix;
    const auto cursor = "tx_cursor_" + suffix;
    write_line("{");
    ++indent_;
    write_line("const tx_int " + first + " = " + emit_expression(*loop.first) + ";");
    write_line("const tx_int " + last + " = " + emit_expression(*loop.last) + ";");
    write_line("if (" + first + " <= " + last + ")");
    write_line("{");
    ++indent_;
    write_line("tx_int " + cursor + " = " + first + ";");
    write_line("while (true)");
    write_line("{");
    ++indent_;
    write_line("const tx_int " + cpp_variable(loop.name) + " = " + cursor + ";");
    emit_statements(loop.body);
    write_line("if (" + cursor + " == " + last + ")");
    write_line("{");
    ++indent_;
    write_line("break;");
    --indent_;
    write_line("}");
    write_line("++" + cursor + ";");
    --indent_;
    write_line("}");
    --indent_;
    write_line("}");
    --indent_;
    write_line("}");
}

void code_generator::emit_for_each(const for_each& loop)
{
    const auto suffix = std::to_string(loop_index_++);
    const auto values = "tx_values_" + suffix;
    write_line("{");
    ++indent_;
    write_line("const tx_array " + values + " = " + emit_expression(*loop.values) + ";");
    write_line("for (const std::any& " + cpp_variable(loop.name) + " : " + values + ")");
    write_line("{");
    ++indent_;
    emit_statements(loop.body);
    --indent_;
    write_line("}");
    --indent_;
    write_line("}");
}

void code_generator::emit_if(const if_statement& branch)
{
    write_line("if (" + emit_expression(*branch.condition) + ")");
    write_line("{");
    ++indent_;
    emit_statements(branch.then_body);
    --indent_;
    write_line("}");
    if (branch.has_else)
    {
        write_line("else");
        write_line("{");
        ++indent_;
        emit_statements(branch.else_body);
        --indent_;
        write_line("}");
    }
}

void code_generator::emit_while(const while_statement& loop)
{
    write_line("while (" + emit_expression(*loop.condition) + ")");
    write_line("{");
    ++indent_;
    emit_statements(loop.body);
    --indent_;
    write_line("}");
}

void code_generator::emit_assignment(const variable_assignment& assignment)
{
    const auto suffix = std::to_string(assignment_index_++);
    const auto target = "tx_target_" + suffix;
    write_line("{");
    ++indent_;
    write_line("auto&& " + target + " = " + emit_lvalue(*assignment.target) + ";");
    const auto value = emit_expression(*assignment.value);
    if (assignment.operation == token_kind::plus_equal)
    {
        const auto result = assignment.target->type == value_type::int_type
            ? "tx_add(" + target + ", " + value + ")"
            : target + " + " + value;
        write_line(target + " = " + result + ";");
    }
    else if (assignment.target->type == value_type::any_type)
    {
        write_line(target + " = tx_box(" + value + ");");
    }
    else
    {
        write_line(target + " = " + value + ";");
    }
    --indent_;
    write_line("}");
}

void code_generator::emit_statement(const statement& item)
{
    if (const auto* declaration = std::get_if<variable_declaration>(&item.data))
    {
        if (declaration->array_length)
        {
            const auto length = "tx_length_" + std::to_string(assignment_index_++);
            write_line("const tx_int " + length + " = " +
                       emit_expression(*declaration->array_length) + ";");
            const auto initial = declaration->initializer
                ? ", " + emit_expression(*declaration->initializer) : "";
            write_line("tx_array " + cpp_variable(declaration->name) +
                       " = tx_make_array(" + length + initial + ");");
            return;
        }
        const auto type = declaration->declared_type.value_or(
            declaration->initializer->type);
        write_line(cpp_type(type) + " " + cpp_variable(declaration->name) +
                   " = " + emit_expression(*declaration->initializer) + ";");
    }
    else if (const auto* assignment = std::get_if<variable_assignment>(&item.data))
    {
        emit_assignment(*assignment);
    }
    else if (const auto* loop = std::get_if<for_loop>(&item.data))
    {
        emit_for(*loop);
    }
    else if (const auto* loop = std::get_if<for_each>(&item.data))
    {
        emit_for_each(*loop);
    }
    else if (const auto* branch = std::get_if<if_statement>(&item.data))
    {
        emit_if(*branch);
    }
    else if (const auto* loop = std::get_if<while_statement>(&item.data))
    {
        emit_while(*loop);
    }
    else if (const auto* result = std::get_if<return_statement>(&item.data))
    {
        write_line(result->value
            ? "return " + emit_expression(*result->value) + ";"
            : "return;");
    }
    else if (const auto* expression_only =
                 std::get_if<expression_statement>(&item.data))
    {
        write_line(emit_expression(*expression_only->value) + ";");
    }
}

void code_generator::emit_statements(const std::vector<stmt_ptr>& statements)
{
    for (const auto& item : statements)
    {
        emit_statement(*item);
    }
}

void code_generator::emit_struct(const struct_decl& definition)
{
    write_line("struct " + cpp_struct(definition.name));
    write_line("{");
    ++indent_;
    for (const auto& field : definition.fields)
    {
        write_line(cpp_type(field.type) + " " + cpp_field(field.name) + ";");
    }
    --indent_;
    write_line("};");
    write_line("");
}

void code_generator::emit_dynamic_fields(const program& source)
{
    write_line("std::any tx_get_field(const std::any& source, std::string_view field)");
    write_line("{");
    ++indent_;
    for (const auto& definition : source.structs)
    {
        const auto struct_type = cpp_struct(definition.name);
        write_line("if (source.type() == typeid(" + struct_type + "))");
        write_line("{");
        ++indent_;
        write_line("const auto& value = std::any_cast<const " + struct_type + "&>(source);");
        for (const auto& field : definition.fields)
        {
            write_line("if (field == \"" + field.name + "\")");
            write_line("{");
            ++indent_;
            write_line("return tx_box(value." + cpp_field(field.name) + ");");
            --indent_;
            write_line("}");
        }
        --indent_;
        write_line("}");
    }
    write_line("throw std::runtime_error(\"字段不存在或对象不是结构体\");");
    --indent_;
    write_line("}");
    write_line("");
}

void code_generator::emit_function(const function_decl& function)
{
    write_line(function_header(function));
    write_line("{");
    ++indent_;
    emit_statements(function.body);
    --indent_;
    write_line("}");
    write_line("");
}

std::string code_generator::generate(const program& source)
{
    output_ << runtime_source;
    indent_ = 0;
    loop_index_ = 0;
    expression_index_ = 0;
    assignment_index_ = 0;
    for (const auto& definition : source.structs)
    {
        emit_struct(definition);
    }
    emit_dynamic_fields(source);
    for (const auto& function : source.functions)
    {
        write_line(function_header(function) + ";");
    }
    write_line("");
    for (const auto& function : source.functions)
    {
        if (!function.external)
        {
            emit_function(function);
        }
    }
    write_line("} // namespace tx_generated");
    write_line("");
    write_line("int main()");
    write_line("{");
    ++indent_;
    write_line("try");
    write_line("{");
    ++indent_;
    write_line("tx_generated::tx_prepare_console();");
    write_line("const auto result = tx_generated::tx_fn_main();");
    write_line("if (result < std::numeric_limits<int>::min() ||");
    write_line("    result > std::numeric_limits<int>::max())");
    write_line("{");
    ++indent_;
    write_line("throw std::runtime_error(\"main 返回码超出平台 int 范围\");");
    --indent_;
    write_line("}");
    write_line("return static_cast<int>(result);");
    --indent_;
    write_line("}");
    write_line("catch (const std::exception& error)");
    write_line("{");
    ++indent_;
    write_line("std::cerr << \"运行错误：\" << error.what() << '\\n';");
    write_line("return 1;");
    --indent_;
    write_line("}");
    --indent_;
    write_line("}");
    return output_.str();
}

} // namespace tx
