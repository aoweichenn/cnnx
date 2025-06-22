//
// Created by aowei on 25-6-21.
//

#ifndef RUNTIME_CNNX_IR_ATTRIBUTE_HPP
#define RUNTIME_CNNX_IR_ATTRIBUTE_HPP

#include <complex>
#include <map>
#include <utility>
#include <vector>
#include <runtime/cnnx/ir/parameter.hpp>

// Attribute 类声明
namespace cnnx
{
    class Attribute
    {
        // type => {
        //  1 => fp32, 2 => fp64, 3 => fp16
        //  4 => i32, 5 => i64, 6 => i16, 7 => i8, 8 => u8
        //  9 => bool
        //  10 => c64, 11 => c128, 12 => c32, 13 => bf16
        // }
    public:
        Attribute() = default;

        Attribute(const std::initializer_list<int>& shape, const std::vector<float>& data);

        [[nodiscard]] size_t element_size() const;
        [[nodiscard]] int element_count() const;
        [[nodiscard]] std::vector<float> get_float32_data() const;
        void set_float32_data(const std::vector<float>& new_data);

    public:
        // type => {
        //  1 => fp32, 2 => fp64, 3 => fp16
        //  4 => i32, 5 => i64, 6 => i16, 7 => i8, 8 => u8
        //  9 => bool
        //  10 => c64, 11 => c128, 12 => c32, 13 => bf16
        // }
        int type{};
        std::vector<int> shape{};
        std::vector<char> data{};
        std::map<std::string, Parameter> params{};
    };

    bool operator==(const Attribute& lhs, const Attribute& rhs);

    Attribute operator+(const Attribute& lhs, const Attribute& rhs);
}

#endif //RUNTIME_CNNX_IR_ATTRIBUTE_HPP
