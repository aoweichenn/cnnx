//
// Created by aowei on 25-6-21.
//

#include <algorithm>
#include <runtime/cnnx/ir/operand.hpp>

// Operand 类中函数的实现
namespace cnnx
{
    // 移除 operand 中对应的 consumer_operator
    void Operand::remove_consumer(const Operator* consumer_operator)
    {
        const auto it = std::find(this->consumers.begin(), this->consumers.end(), consumer_operator);
        if (it != this->consumers.end())
        {
            this->consumers.erase(it);
        }
    }
}
