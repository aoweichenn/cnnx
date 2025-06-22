//
// Created by aowei on 25-6-21.
//

// Operator 类声明，操作符

#include <algorithm>
#include <runtime/cnnx/ir/operator.hpp>

// Operator 类声明，操作符
namespace cnnx
{
    // 判断是否有某个 param
    bool Operator::has_param(const std::string& key) const
    {
        return this->params.find(key) != this->params.end();
    }

    // 判断是否有某个 attr
    bool Operator::has_attr(const std::string& key) const
    {
        return this->attrs.find(key) != this->attrs.end();
    }

    // 判断是否有某个 input
    bool Operator::has_input(const std::string& key) const
    {
        const auto it = std::find(this->input_names.begin(), this->input_names.end(), key);
        return it != this->input_names.end();
    }

    // 如果有对应的 input name，就返回对应的操作数
    Operand* Operator::named_input(const std::string& key)
    {
        for (size_t i = 0; i < this->inputs.size(); ++i)
        {
            if (this->input_names[i] == key)
            {
                return this->inputs[i];
            }
        }
        return nullptr;
    }

    // 如果有对应的 input name，就返回对应的操作数
    const Operand* Operator::named_input(const std::string& key) const
    {
        for (size_t i = 0; i < this->inputs.size(); ++i)
        {
            if (this->input_names[i] == key)
            {
                return this->inputs[i];
            }
        }
        return nullptr;
    }
}
