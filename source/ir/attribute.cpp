//
// Created by aowei on 25-6-21.
//
#include <cstring>
#include <algorithm>
#include <runtime/cnnx/ir/attribute.hpp>
#include <runtime/cnnx/utils/fp16_converter.hpp>

// 辅助静态函数
namespace cnnx
{
    // 这个函数不是指的 parameter 里面的 type
    static bool type_is_integer(const int type)
    {
        if (type == 1) return false;
        if (type == 2) return false;
        if (type == 3) return false;
        if (type == 4) return true;
        if (type == 5) return true;
        if (type == 6) return true;
        if (type == 7) return true;
        if (type == 8) return true;
        if (type == 9) return true;
        if (type == 10) return false;
        if (type == 11) return false;
        if (type == 12) return false;
        if (type == 13) return false;
        return false;
    }

    // 对应本地命名的类型字符串
    static const char* type_to_string(const int type)
    {
        if (type == 1) return "f32";
        if (type == 2) return "f64";
        if (type == 3) return "f16";
        if (type == 4) return "i32";
        if (type == 5) return "i64";
        if (type == 6) return "i16";
        if (type == 7) return "i8";
        if (type == 8) return "u8";
        if (type == 9) return "bool";
        if (type == 10) return "c64";
        if (type == 11) return "c128";
        if (type == 12) return "c32";
        if (type == 13) return "bf16";
        return "null";
    }

    // 对应 numpy 里面的类型字符串
    static const char* type_to_numpy_string(const int type)
    {
        if (type == 1) return "float32";
        if (type == 2) return "float64";
        if (type == 3) return "float16";
        if (type == 4) return "int32";
        if (type == 5) return "int64";
        if (type == 6) return "int16";
        if (type == 7) return "int8";
        if (type == 8) return "uint8";
        if (type == 9) return "bool";
        if (type == 10) return "csingle";
        if (type == 11) return "cdouble";
        if (type == 12) return "chalf";
        if (type == 13) return "bfloat16";
        return "null";
    }

    // 对应 torch 里面的类型字符串
    static const char* type_to_dtype_string(const int type)
    {
        if (type == 1) return "torch.float";
        if (type == 2) return "torch.double";
        if (type == 3) return "torch.half";
        if (type == 4) return "torch.int";
        if (type == 5) return "torch.long";
        if (type == 6) return "torch.short";
        if (type == 7) return "torch.int8";
        if (type == 8) return "torch.uint8";
        if (type == 9) return "torch.bool";
        if (type == 10) return "torch.complex64";
        if (type == 11) return "torch.complex128";
        if (type == 12) return "torch.complex32";
        if (type == 13) return "torch.bfloat16";
        return "null";
    }

    // 返回对应类型的所占用的字节数
    // TODO: 这里的 complex 类型占用字节数是不是有问题
    static size_t type_to_element_size(const int type)
    {
        if (type == 1) return 4;
        if (type == 2) return 8;
        if (type == 3) return 2;
        if (type == 4) return 4;
        if (type == 5) return 8;
        if (type == 6) return 2;
        if (type == 7) return 1;
        if (type == 8) return 1;
        if (type == 9) return 1;
        if (type == 10) return 8;
        if (type == 11) return 16;
        if (type == 12) return 4;
        if (type == 13) return 2;
        return 0; // null
    }

    // 把自定义的类型字符串转为 type 值
    static int string_to_type(const char* s)
    {
        if (strcmp(s, "f32") == 0) return 1;
        if (strcmp(s, "f64") == 0) return 2;
        if (strcmp(s, "f16") == 0) return 3;
        if (strcmp(s, "i32") == 0) return 4;
        if (strcmp(s, "i64") == 0) return 5;
        if (strcmp(s, "i16") == 0) return 6;
        if (strcmp(s, "i8") == 0) return 7;
        if (strcmp(s, "u8") == 0) return 8;
        if (strcmp(s, "bool") == 0) return 9;
        if (strcmp(s, "c64") == 0) return 10;
        if (strcmp(s, "c128") == 0) return 11;
        if (strcmp(s, "c32") == 0) return 12;
        if (strcmp(s, "bf16") == 0) return 13;
        return 0; // null
    }
}

// Attribute 类中函数的实现
namespace cnnx
{
    // 初始化 Attribute
    Attribute::Attribute(const std::initializer_list<int>& shape, const std::vector<float>& data)
    {
        this->type = 1;
        this->shape = shape;
        if (!this->shape.empty())
        {
            // 元素数量和元素的字节大小的乘积就是数据的字节大小
            this->data.resize(this->element_count() * type_to_element_size(this->type));
            memcpy(reinterpret_cast<void*>(this->data.data()), reinterpret_cast<const void*>(data.data()),
                   this->data.size());
        }
    }

    // 返回 Attribute 代表的元素的类型的字节大小
    size_t Attribute::element_size() const
    {
        return type_to_element_size(this->type);
    }

    // 通过似 [1,3,3,9] 中解析元素数量
    int Attribute::element_count() const
    {
        if (this->shape.empty())
        {
            return 0;
        }
        int elements_num = this->shape[0];
        // 这里的 shape 从类似 [1,3,3,9] 中解析而来
        for (size_t i = 1; i < this->shape.size(); ++i)
        {
            elements_num *= this->shape[i];
        }
        return elements_num;
    }

    // 设置数据 (float 簇, eg.fp16, fp32, fp64)
    void Attribute::set_float32_data(const std::vector<float>& new_data)
    {
        // 先重新组装空间
        this->data.resize(new_data.size() * this->element_size());
        //
        if (this->type == 1)
        {
            // Attribute 的数据类型是 fp32，正常复制就行
            // 拷贝数据到 this->data 所处的空间里面
            memcpy(reinterpret_cast<void*>(this->data.data()),
                   reinterpret_cast<const void*>(new_data.data()),
                   this->data.size());
        }
        else if (this->type == 2)
        {
            // Attribute 的数据类型是 fp64，就需要类型转化
            // 按照 doule* 的方式解析内存数据
            auto* ptr = reinterpret_cast<double*>(this->data.data());
            for (size_t i = 0; i < this->data.size(); ++i)
            {
                ptr[i] = new_data[i];
            }
        }
        else if (this->type == 3)
        {
            // Attribute 的数据类型是 fp16，就需要类型转化
            // 按照 char* 的方式解析内存数据
            auto* ptr = reinterpret_cast<unsigned short*>(this->data.data());
            for (size_t i = 0; i < this->data.size(); ++i)
            {
                // 使用自定义转换将 fp32 转为 fp16
                ptr[i] = float32_to_float16(new_data[i]);
            }
        }
        else
        {
            // fp32 没法转换为其他数据类型
            fprintf(stderr, "cannot convert float32 data to type %d\n", this->type);
        }
    }

    // 根据 Attribute 数据 (float 簇, eg.fp16, fp32, fp64) 设置
    std::vector<float> Attribute::get_float32_data() const
    {
        std::vector<float> fp32vec(element_count());
        if (this->type == 1)
        {
            // Attribute 自身为 fp64 类型的数据
            // 使用内存拷贝功能，防止直接赋值导致的对类内数据的修改
            memcpy(reinterpret_cast<void*>(fp32vec.data()),
                   reinterpret_cast<const void*>(this->data.data()),
                   this->data.size());
        }
        else if (this->type == 2)
        {
            // Attribute 自身为 fp64 类型的数据
            // 使用内存拷贝功能，防止直接赋值导致的对类内数据的修改
            const auto* ptr = reinterpret_cast<const double*>(this->data.data());
            for (size_t i = 0; i < this->data.size(); ++i)
            {
                fp32vec[i] = static_cast<float>(ptr[i]);
            }
        }
        else if (this->type == 3)
        {
            // Attribute 自身为 fp16 类型的数据
            // 使用内存拷贝功能，防止直接赋值导致的对类内数据的修改
            const auto* ptr = reinterpret_cast<const unsigned short*>(this->data.data());
            for (size_t i = 0; i < this->data.size(); ++i)
            {
                // 使用自定义转换将 fp16 转为 fp32
                fp32vec[i] = float16_to_float32(ptr[i]);
            }
        }
        return fp32vec;
    }

    // 这里的 operator + 实际上等价于 concat 第一个 dim
    Attribute operator+(const Attribute& lhs, const Attribute& rhs)
    {
        Attribute result;
        if (lhs.type != rhs.type)
        {
            // 左右变量类型不匹配
            fprintf(stderr, "concat attribute type  mismatch\n");
            return result;
        }
        if (lhs.shape.size() != rhs.shape.size())
        {
            // 左右变量 shape 大小不匹配
            fprintf(stderr, "concat attribute shape rank  mismatch\n");
            return result;
        }
        for (int i = 1; i < static_cast<int>(lhs.shape.size()); ++i)
        {
            if (lhs.shape[i] != rhs.shape[i])
            {
                // 左右变量 shape 中的值不匹配
                fprintf(stderr, "concat attribute shape mismatch\n");
                return result;
            }
        }
        result.type = lhs.type;
        // shape 是 [batch ,...]，所以 batch 的值是两个相加，其他的值却必须相等
        // 等价于 concat 第一个 dim
        result.shape = lhs.shape;
        result.shape[0] = rhs.shape[0];

        // 合并数据，左边的数据在前面，右边的数据在后面（内存上）
        result.data.resize(lhs.data.size() + rhs.data.size());
        memcpy(result.data.data(), lhs.data.data(), lhs.data.size());
        memcpy(result.data.data() + lhs.data.size(), rhs.data.data(), rhs.data.size());

        return result;
    }

    // 判断两个 Attribute 里面的所有变量的值是不是相等
    bool operator==(const Attribute& lhs, const Attribute& rhs)
    {
        if (lhs.type != rhs.type) return false;
        if (lhs.type == 0) return true;
        // vector 的比较方式是值比较，先比较 vec 大小，再遍历元素比较
        if (lhs.shape != rhs.shape) return false;
        // vector 的比较方式是值比较，先比较 vec 大小，再遍历元素比较
        if (lhs.data != rhs.data) return false;
        return true;
    }
}
