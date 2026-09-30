#include "backend/llvm/target_attributes.hpp"

#include <sstream>
#include <stdexcept>

namespace tx
{

std::string finish_llvm_module(const std::string& module)
{
    std::istringstream input(module);
    std::ostringstream output;
    std::string line;
    while (std::getline(input, line))
    {
        if (line.starts_with("define "))
        {
            // 本发射器的函数头均占一行；在统一出口为用户函数及所有生成桥
            // 附加同一目标属性，防止遗忘某种适配器而阻断跨模块内联。
            const auto body = line.rfind('{');
            if (body == std::string::npos)
            {
                throw std::logic_error("LLVM 函数头缺少代码块起点");
            }
            line.insert(body, "#0 ");
        }
        output << line << '\n';
    }
    output << "attributes #0 = { uwtable \"target-cpu\"=\"x86-64\" "
        "\"target-features\"=\"+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87\" }\n";
    return output.str();
}

} // namespace tx
