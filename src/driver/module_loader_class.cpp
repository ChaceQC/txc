#include "driver/module_loader_class.hpp"

namespace tx
{

bool same_parameter_types(const function_decl& left,
                          const function_decl& right)
{
    if (left.parameters.size() != right.parameters.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < left.parameters.size(); ++index)
    {
        if (left.parameters[index].type != right.parameters[index].type ||
            left.parameters[index].kind != right.parameters[index].kind)
        {
            return false;
        }
    }
    return true;
}

void copy_parameter_defaults(const std::vector<parameter>& declaration,
                             std::vector<parameter>& implementation)
{
    for (std::size_t index = 0; index < declaration.size(); ++index)
    {
        if (implementation[index].default_value)
        {
            throw compile_error(implementation[index].position,
                                "配对实现不能重复声明参数默认值");
        }
        implementation[index].default_value = declaration[index].default_value;
    }
}

bool same_class_layout(const class_decl& left, const class_decl& right)
{
    if (left.bases != right.bases ||
        left.is_interface != right.is_interface ||
        left.is_abstract != right.is_abstract ||
        left.fields.size() != right.fields.size() ||
        left.methods.size() != right.methods.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < left.fields.size(); ++index)
    {
        const auto& a = left.fields[index];
        const auto& b = right.fields[index];
        if (a.name != b.name || a.type != b.type || a.access != b.access)
        {
            return false;
        }
    }
    for (std::size_t index = 0; index < left.methods.size(); ++index)
    {
        const auto& a = left.methods[index];
        const auto& b = right.methods[index];
        if (a.name != b.name || a.return_type != b.return_type ||
            a.access != b.access || a.is_virtual != b.is_virtual ||
            a.is_override != b.is_override ||
            a.is_abstract != b.is_abstract ||
            a.operator_kind != b.operator_kind ||
            !same_parameter_types(a, b))
        {
            return false;
        }
        for (std::size_t parameter = 0; parameter < a.parameters.size(); ++parameter)
        {
            if (a.parameters[parameter].name != b.parameters[parameter].name)
            {
                return false;
            }
        }
    }
    return true;
}

bool same_struct_layout(const struct_decl& left, const struct_decl& right)
{
    if (left.fields.size() != right.fields.size() ||
        left.methods.size() != right.methods.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < left.fields.size(); ++index)
    {
        const auto& a = left.fields[index];
        const auto& b = right.fields[index];
        if (a.name != b.name || a.type != b.type)
        {
            return false;
        }
    }
    for (std::size_t index = 0; index < left.methods.size(); ++index)
    {
        const auto& a = left.methods[index];
        const auto& b = right.methods[index];
        if (a.name != b.name || a.operator_kind != b.operator_kind ||
            a.return_type != b.return_type ||
            !same_parameter_types(a, b))
        {
            return false;
        }
        for (std::size_t parameter = 0; parameter < a.parameters.size(); ++parameter)
        {
            if (a.parameters[parameter].name != b.parameters[parameter].name)
            {
                return false;
            }
        }
    }
    return true;
}

} // namespace tx
