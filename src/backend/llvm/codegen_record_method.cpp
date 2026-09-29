#include "backend/llvm/codegen.hpp"

namespace tx
{

void llvm_code_generator::emit_record_method_adapter(const function_decl& function,
    const std::string& symbol, std::size_t index)
{
    // 虚表、析构和原生边界继续使用原签名；静态 TX 调用可直接携带已知对象视图。
    std::string parameters = "ptr %tx_context, ptr %arg0";
    std::string arguments = "ptr %tx_context, ptr %arg0, ptr %view";
    for (std::size_t parameter_index = 0; parameter_index < function.parameters.size(); ++parameter_index)
    {
        const auto& parameter = function.parameters[parameter_index];
        const auto value = llvm_type(parameter_abi_type(parameter), parameter.position) +
            " %arg" + std::to_string(parameter_index + 1);
        parameters += ", " + value;
        arguments += ", " + value;
    }
    const auto result = llvm_type(function.return_type, function.position);
    const auto name = function_name(symbol, index);
    module_ << "define " << result << ' ' << name << '(' << parameters << ") {\nentry:\n"
            << "  %view = call ptr @" << (classes_.contains(function.owner_class)
                ? "txrt_record_class_view" : "txrt_record_struct_view") << "(ptr %arg0)\n"
            << "  " << (result == "void" ? "" : "%result = ") << "call "
            << result << ' ' << name << "_record(" << arguments << ")\n"
            << (result == "void" ? "  ret void\n" : "  ret " + result + " %result\n")
            << "}\n\n";
}

} // namespace tx
