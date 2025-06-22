//
// Created by aowei on 25-6-21.
//

#ifndef RUNTIME_CNNX_IR_OPERAND_HPP
#define RUNTIME_CNNX_IR_OPERAND_HPP

#include <complex>
#include <map>
#include <vector>
#include <runtime/cnnx/ir/parameter.hpp>
// Operand 类声明，操作数
// TODO: 分析设计思路
namespace cnnx
{
    class Operator;

    class Operand
    {
    public:
        Operator* producer{};
        std::vector<Operator*> consumers{};
        std::vector<int> shape{};
        std::map<std::string, Parameter> params{};
        int type;
        // keep std::string typed member the last for cross cxxabi compatibility
        std::string name;

    public:
        void remove_consumer(const Operator* consumer_operator);

    private:
        friend class Graph;

        Operand(): type(0)
        {
        }
    };
}

#endif //RUNTIME_CNNX_IR_OPERAND_HPP
