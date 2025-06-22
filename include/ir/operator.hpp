//
// Created by aowei on 25-6-21.
//

#ifndef RUNTIME_CNNX_IR_OPERATOR_HPP
#define RUNTIME_CNNX_IR_OPERATOR_HPP

#include <complex>
#include <map>
#include <vector>
#include <runtime/cnnx/ir/parameter.hpp>
#include <runtime/cnnx/ir/attribute.hpp>
#include <runtime/cnnx/ir/operand.hpp>

// Operator 类声明，操作符
// TODO: 分析设计思路
namespace cnnx
{
    class Operator
    {
    public:
        Operator() = default;
        [[nodiscard]] bool has_param(const std::string& key) const;
        [[nodiscard]] bool has_attr(const std::string& key) const;
        [[nodiscard]] bool has_input(const std::string& key) const;

        Operand* named_input(const std::string& key);
        [[nodiscard]] const Operand* named_input(const std::string& key) const;

    public:
        std::vector<Operand*> inputs;
        std::vector<Operand*> outputs;
        std::vector<std::string> input_names;
        std::map<std::string, Parameter> params;
        std::map<std::string, Attribute> attrs;

        // keep std::string typed member the last for cross cxxabi compatibility
        std::string type;
        std::string name;

    private:
        friend class Graph;
    };
}

#endif //RUNTIME_CNNX_IR_OPERATOR_HPP
