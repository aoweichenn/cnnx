//
// Created by aowei on 25-6-21.
//

#ifndef RUNTIME_CNNX_IR_GRAPH_HPP
#define RUNTIME_CNNX_IR_GRAPH_HPP
#include <complex>
#include <vector>
#include <runtime/cnnx/ir/operand.hpp>
#include <runtime/cnnx/ir/operator.hpp>

// Graph 类声明
namespace cnnx
{
    class Graph
    {
    public:
        Graph();
        ~Graph();

        int load(const std::string& param_path, const std::string& bin_path);
        [[nodiscard]] int save(const std::string& param_path, const std::string& bin_path) const;

        int parse(const std::string& param);

        Operator* new_operator(const std::string& type, const std::string& name);
        Operator* new_operator_before(const std::string& type, const std::string& name,
                                      const Operator* current_operator);
        Operator* new_operator_after(const std::string& type, const std::string& name,
                                     const Operator* current_operator);
        Operand* new_operand(const std::string& name);
        Operand* get_operand(const std::string& name);
        [[nodiscard]] const Operand* get_operand(const std::string& name) const;

    public:
        std::vector<Operator*> operators;
        std::vector<Operand*> operands;

    private:
        Graph(const Graph& rhs);
        Graph& operator =(const Graph& rhs);
    };
}

#endif //RUNTIME_CNNX_IR_GRAPH_HPP
