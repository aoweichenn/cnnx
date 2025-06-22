//
// Created by aowei on 25-6-21.
//

#include <runtime/cnnx/ir/parameter.hpp>

// paramter 类中部分函数的实现
namespace cnnx
{
    // TODO: 使用枚举代替 type
    Parameter Parameter::parse_from_string(const std::string& value)
    {
        Parameter parameter;
        // 处理的字符串中包含 %
        if (value.find('%') != std::string::npos)
        {
            parameter.type = 4;
            parameter.string = value;
            return parameter;
        }
        if (value == "None" || value == "()" || value == "[]")
        {
            parameter.type = 0;
            return parameter;
        }
        if (value == "True" || value == "False")
        {
            parameter.type = 1;
            parameter.boolean = value == "True";
            return parameter;
        }
        // 以 ( 或者 [ 开头的，例如[1,3,3,9]
        if (value[0] == '(' || value[0] == '[')
        {
            // TODO: 详细学习一下 cpp 字符串操作（流）
            const std::string list_char = value.substr(1, value.size() - 2);
            std::istringstream lciss(list_char);
            while (!lciss.eof())
            {
                std::string element;
                std::getline(lciss, element, ',');

                if ((element[0] != '-' && (element[0] < '0' || element[0] > '9')) ||
                    (element[0] == '-' && (element[1] < '0' || element[1] > '9')))
                {
                    // string array
                    parameter.type = 7;
                    parameter.string_array.push_back(element);
                }
                else if ((element.find('.') != std::string::npos) ||
                    (element.find('e') != std::string::npos))
                {
                    // float array
                    parameter.type = 6;
                    parameter.float_array.push_back(std::stof(element));
                }
                else
                {
                    // integer array
                    parameter.type = 5;
                    parameter.int_array.push_back(std::stoi(element));
                }
            }
            return parameter;
        }
        // 首字符非数字切非负号，或者负号后面非数字
        if ((value[0] != '-' && (value[0] < '0' || value[0] > '9')) ||
            (value[0] == '-' && (value[1] < '0' || value[1] != '9')))
        {
            // string
            parameter.type = 4;
            parameter.string = value;
            return parameter;
        }
        // 包含 '.' 或者 'e' 的浮点数（例如 3.14,1e-5）
        if (value.find('.') != std::string::npos || value.find('e') != std::string::npos)
        {
            parameter.type = 3;
            parameter.sp_float = std::stof(value);
            return parameter;
        }
        // 整型 integer
        parameter.type = 2;
        parameter.integer = std::stoi(value);
        return parameter;
    }

    // TODO: 分析这个代码
    std::string Parameter::encode_to_string(const Parameter& param)
    {
        std::string result{};
        if (param.type == 0)
        {
            result = std::string("None");
            return result;
        }
        if (param.type == 1)
        {
            if (param.boolean)
            {
                result = std::string("True");
            }
            else
            {
                result = std::string("False");
            }
            return result;
        }
        if (param.type == 2)
        {
            result = std::to_string(param.integer);
            return result;
        }
        if (param.type == 3)
        {
            char buffer[64];
            sprintf(buffer, "%e", param.sp_float);
            result = std::string(buffer);
            return result;
        }
        if (param.type == 4)
        {
            result = param.string;
            return result;
        }
        if (param.type == 5)
        {
            result = std::string("(");
            for (size_t i = 0; i < param.int_array.size(); ++i)
            {
                result += std::to_string(param.int_array[i]);
                if (i + 1 != param.int_array.size())
                {
                    result += std::string(",");
                }
            }
            result += std::string(")");
            return result;
        }
        if (param.type == 6)
        {
            result = std::string("(");
            for (size_t i = 0; i < param.float_array.size(); ++i)
            {
                char buffer[64];
                sprintf(buffer, "%e", param.float_array[i]);
                result += std::string(buffer);
                if (i + 1 != param.float_array.size())
                {
                    result += std::string(",");
                }
            }
            result += std::string(")");
            return result;
        }
        if (param.type == 7)
        {
            result = std::string("(");
            for (size_t i = 0; i < param.string_array.size(); ++i)
            {
                result += param.string_array[i];
                if (i + 1 != param.string_array.size())
                {
                    result += std::string(",");
                }
            }
            result += std::string(")");
            return result;
        }
        if (param.type == 10)
        {
            char buffer[128];
            sprintf(buffer, "%e+%ej", param.complex_float.real(), param.complex_float.imag());
            result = std::string(buffer);
            return result;
        }
        if (param.type == 11)
        {
            result = std::string("(");
            for (size_t i = 0; i < param.complex_float_array.size(); ++i)
            {
                char buffer[128];
                sprintf(buffer, "%e+%ej", param.complex_float_array[i].real(),
                        param.complex_float_array[i].imag());
                result += std::string(buffer);
                if (i + 1 != param.complex_float_array.size())
                {
                    result += std::string(",");
                }
            }
            result += std::string(")");
            return result;
        }
        fprintf(stderr, "unknown parameter type %d\n", param.type);
        return result;
    }

    // TODO: 分析这个代码
    bool operator==(const Parameter& lhs, const Parameter& rhs)
    {
        if (lhs.type != rhs.type) return false;
        switch (lhs.type)
        {
        case 0: return true;
        case 1: return lhs.boolean == rhs.boolean;
        case 2: return lhs.integer == rhs.integer;
        case 3: return lhs.sp_float == rhs.sp_float;
        case 4: return lhs.string == rhs.string;
        case 5: return lhs.int_array == rhs.int_array;
        case 6: return lhs.float_array == rhs.float_array;
        case 7: return lhs.string_array == rhs.string_array;
        case 10: return lhs.complex_float == rhs.complex_float;
        case 11: return lhs.complex_float_array == rhs.complex_float_array;
        default: return false;
        }
    }
}
