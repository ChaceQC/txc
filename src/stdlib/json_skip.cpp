#include "stdlib/json_parser.hpp"

#include <unordered_set>

namespace tx_generated
{

void json_parser::skip_value(std::size_t depth)
{
    if (depth > max_depth_)
    {
        input_.fail("depth_limit", "JSON 嵌套超过深度上限");
    }
    const bool object = input_.peek() == '{';
    if (!object && input_.peek() != '[')
    {
        (void)parse_value(depth);
        return;
    }
    (void)input_.get();
    skip_space();
    const char close = object ? '}' : ']';
    if (input_.take(close))
    {
        return;
    }
    // 忽略值仍需验证完整语法及所有层级的重复键，只保存当前层键集合。
    std::unordered_set<std::string> keys;
    while (true)
    {
        skip_space();
        if (object)
        {
            if (input_.peek() != '"')
            {
                input_.fail("invalid_syntax", "对象字段名必须是字符串");
            }
            auto key = parse_string();
            if (reject_duplicate_keys_ && !keys.insert(std::move(key)).second)
            {
                input_.fail("duplicate_key", "serde JSON 对象包含重复字段名");
            }
            skip_space();
            if (!input_.take(':'))
            {
                input_.fail("invalid_syntax", "对象字段名后需要冒号");
            }
            skip_space();
        }
        skip_value(depth + 1);
        skip_space();
        if (input_.take(close))
        {
            return;
        }
        if (!input_.take(','))
        {
            input_.fail("invalid_syntax", "元素后需要逗号或容器结束符");
        }
    }
}

} // namespace tx_generated
