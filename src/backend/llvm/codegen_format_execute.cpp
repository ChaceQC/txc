#include "backend/llvm/codegen.hpp"
#include "backend/llvm/codegen_format_plan.hpp"

namespace tx
{

std::string llvm_code_generator::static_format_executor(const call_expression& call,
    const std::vector<static_format_part>& plan,
    const std::vector<std::optional<std::string>>& bytes, const std::string& steps)
{
    const auto symbol = "@tx_format_static_" + std::to_string(next_string_++);
    std::ostringstream body;
    body << "define internal void " << symbol << "(ptr %output, ptr %arguments) uwtable {\nentry:\n";
    std::size_t index = 0;
    for (const auto& part : plan)
    {
        if (!part.argument)
        {
            continue;
        }
        const auto argument = *part.argument;
        const auto& type = call.arguments[argument].value->type;
        const auto suffix = bytes[argument] ? "literal" : type == value_type::int_type ? "i64" :
            type == value_type::float_type ? "f64" : type == value_type::bool_type ? "bool" : "str";
        const auto callback = std::string("@tx_format_") +
            (plain_format_part(part, type) ? "fast_" : "argument_") + suffix;
        const auto id = std::to_string(index);
        // 步骤表只作为只读规格常量使用；目标和参数位置均已静态解析。
        body << "  %arg" << id << " = getelementptr i64, ptr %arguments, i64 " << argument << '\n'
             << "  %bits" << id << " = load i64, ptr %arg" << id << '\n'
             << "  %spec" << id << " = getelementptr { ptr, i64, [7 x i64], i64, ptr, i64 }, ptr "
             << steps << ", i64 " << index << ", i32 2\n"
             << "  call void " << callback << "(ptr %output, i64 %bits" << id << ", ptr %spec"
             << id << ", i8 " << static_cast<unsigned char>(part.conversion) + 0 << ")\n";
        if (!part.tail.empty())
        {
            body << "  call void @tx_format_tail(ptr %output, ptr " << global_bytes(part.tail)
                 << ", i64 " << part.tail.size() << ")\n";
        }
        ++index;
    }
    body << "  ret void\n}\n\n";
    module_ << body.str();
    return symbol;
}

std::string llvm_code_generator::static_format_arguments(const std::vector<ir_value>& values,
    const std::vector<std::optional<std::string>>& bytes)
{
    const auto storage = "%slot" + std::to_string(next_slot_++);
    allocations_ << "  " << storage << " = alloca [" << values.size() << " x i64], align 8\n";
    for (std::size_t index = 1; index < values.size(); ++index)
    {
        const auto& value = values[index];
        auto bits = value.text;
        if (bytes[index])
        {
            bits = "ptrtoint (ptr " + global_bytes(*bytes[index]) + " to i64)";
        }
        else if (value.type != value_type::int_type)
        {
            bits = temporary();
            const auto operation = value.type == value_type::float_type ? "bitcast double" :
                value.type == value_type::bool_type ? "zext i1" : "ptrtoint ptr";
            write_instruction(bits + " = " + operation + " " + value.text + " to i64");
        }
        const auto address = temporary();
        write_instruction(address + " = getelementptr i64, ptr " + storage +
                          ", i64 " + std::to_string(index));
        write_instruction("store i64 " + bits + ", ptr " + address);
    }
    return storage;
}

std::string llvm_code_generator::static_format_steps(const call_expression& call,
    const std::vector<static_format_part>& plan, const std::vector<std::optional<std::string>>& bytes)
{
    const auto symbol = "@.format_steps." + std::to_string(next_string_++);
    constexpr auto layout = "{ ptr, i64, [7 x i64], i64, ptr, i64 }";
    std::string entries;
    std::size_t count = 0;
    for (const auto& part : plan)
    {
        if (!part.argument)
        {
            continue;
        }
        const auto index = *part.argument;
        const auto& type = call.arguments[index].value->type;
        const auto suffix = bytes[index] ? "literal" : type == value_type::int_type ? "i64" :
            type == value_type::float_type ? "f64" : type == value_type::bool_type ? "bool" : "str";
        const auto callback = std::string("@tx_format_") +
            (plain_format_part(part, type) ? "fast_" : "argument_") + suffix;
        auto spec = part.spec;
        if (bytes[index])
        {
            spec.precision = bytes[index]->size();
        }
        entries += (count++ ? ", " : "") + std::string(layout) + " { ptr " + callback +
            ", i64 " + std::to_string(index) + ", [7 x i64] [i64 " + std::to_string(spec.fill) +
            ", i64 " + std::to_string(spec.align) + ", i64 " + std::to_string(spec.sign) +
            ", i64 " + std::to_string(spec.type) + ", i64 " + std::to_string(spec.zero) +
            ", i64 " + std::to_string(spec.width) + ", i64 " + std::to_string(spec.precision) +
            "], i64 " + std::to_string(part.conversion) + ", ptr " + global_bytes(part.tail) +
            ", i64 " + std::to_string(part.tail.size()) + " }";
    }
    globals_ << symbol << " = private constant [" << count << " x " << layout << "] [" << entries << "]\n";
    return symbol;
}

} // namespace tx
