//
// Created by aowei on 25-6-21.
//
#include <fstream>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <runtime/cnnx/ir/graph.hpp>
#include <runtime/cnnx/utils/storezip.hpp>

namespace cnnx
{
    constexpr auto BLANK_SPACE = " ";
    constexpr auto LINE_FEED = "\n";
}

// 辅助静态函数 => 主要参与类型之间的转换
namespace cnnx
{
    // 把自定义的类型字符串转为 type 值
    static int string_to_type(const char* string)
    {
        if (strcmp(string, "f32") == 0) return 1;
        if (strcmp(string, "f64") == 0) return 2;
        if (strcmp(string, "f16") == 0) return 3;
        if (strcmp(string, "i32") == 0) return 4;
        if (strcmp(string, "i64") == 0) return 5;
        if (strcmp(string, "i16") == 0) return 6;
        if (strcmp(string, "i8") == 0) return 7;
        if (strcmp(string, "u8") == 0) return 8;
        if (strcmp(string, "bool") == 0) return 9;
        if (strcmp(string, "c64") == 0) return 10;
        if (strcmp(string, "c128") == 0) return 11;
        if (strcmp(string, "c32") == 0) return 12;
        if (strcmp(string, "bf16") == 0) return 13;
        return 0; // null
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
}

// 静态辅助函数 => 主要参与加载各类参数和属性和输入加载
namespace cnnx
{
    // 加载参数配置
    static void load_parameter(Operator* op, const std::string& key, const std::string& value)
    {
        op->params[key] = Parameter::parse_from_string(value);
    }

    // 加载指定的输入 input
    static void load_input_key(Operator* op, const std::string& key, const std::string& value)
    {
        // TODO: 为什么这么做
        op->input_names.resize(op->inputs.size());
        // TODO: 这一步在做什么
        for (size_t i = 0; i < op->inputs.size(); ++i)
        {
            const Operand* operand = op->inputs[i];
            if (operand->name == value)
            {
                op->input_names[i] = key;
                break;
            }
        }
    }

    // 加载 shape
    // TODO: 详细分析这个代码
    static void load_shape(const Operator* op, const std::string& key, const std::string& value)
    {
        Operand* operand = nullptr;
        // 这里遍历所有输入的操作数（operand）
        // 当 key 的值能在 operand 里面找到时退出循环
        // 不过有可能遍历整个输入 operand 都没找到对应的名字
        for (const auto opd : op->inputs)
        {
            if (opd->name == key)
            {
                operand = opd;
                break;
            }
        }
        // 当遍历整个 input operand 里面找不到时，就进入下面这个 if 里面
        // 这个时候会判断 key 在不在 output 里面
        // 如果存在的话就会把值赋给 operand
        if (!operand)
        {
            for (const auto opd : op->outputs)
            {
                if (opd->name == key)
                {
                    operand = opd;
                    break;
                }
            }
        }
        // 如果 key 在 input 和 output 里面都找不到的话就输出错误日志
        if (!operand)
        {
            fprintf(stderr, "no such operand %s for operator %s\n", key.c_str(), op->name.c_str());
            return;
        }

        // type
        // TODO: 解析这段代码
        const std::string typestr = value.substr(value.find_last_of(')') + 1);
        operand->type = string_to_type(typestr.c_str());

        // 获取 shape
        // TODO: 解析这段代码
        const std::string lc = value.substr(1, value.find_last_of(')') - 1);
        std::istringstream lcss(lc);

        operand->shape.clear();
        while (!lcss.eof())
        {
            std::string element;
            std::getline(lcss, element, ',');
            if (element == "?")
            {
                operand->shape.push_back(-1);
            }
            else if (element[0] == '%')
            {
                // encode %abc as symbolic tag
                operand->shape.push_back(-233);
                const size_t index = operand->shape.size() - 1;
                const std::string param = element.substr(1);
                operand->params[std::string("__shape__") + std::to_string(index)] = Parameter(param);
            }
            else
            {
                int i = std::stoi(element);
                operand->shape.push_back(i);
            }
        }
    }

    // 加载 attribute
    // TODO: 详细分析这个代码
    static void load_attribute(Operator* op, const std::string& key, const std::string& value,
                               StoreZipReader& szr)
    {
        Attribute& attr = op->attrs[key];
        // type
        const std::string typestr = value.substr(value.find_last_of(')') + 1);
        attr.type = string_to_type(typestr.c_str());

        // 属性为 0 时（nullptr）
        if (attr.type == 0) return;

        // shape
        const std::string lc = value.substr(1, value.find_last_of(')') - 1);
        std::istringstream lcss(lc);

        attr.shape.clear();
        while (!lcss.eof())
        {
            std::string element;
            std::getline(lcss, element, ',');

            int i = std::stoi(element);
            attr.shape.push_back(i);
        }

        if (attr.shape.empty()) return;

        size_t size = 1;
        for (const int ele : attr.shape)
        {
            size *= ele;
        }
        // 计算空间大小
        const size_t bytesize = size * type_to_element_size(attr.type);
        const std::string filename = op->name + "." + key;
        const size_t filesize = szr.get_file_size(filename);
        // 文件不存在
        if (filesize == 0) return;
        if (filesize != bytesize)
        {
            fprintf(stderr, "file size not match expect %lu but got %lu\n", bytesize, filesize);
        }
        attr.data.resize(bytesize);
        szr.read_file(filename, reinterpret_cast<char*>(attr.data.data()));
    }
}

// 静态辅助函数，辅解析 param => 处理 graph::save 的辅助函数
namespace cnnx
{
    // 写入魔数到 cnnx param 文件当中
    static void write_magic_number(FILE* param_fp)
    {
        fprintf(param_fp, "7767517%s", LINE_FEED);
    }

    // 写入总的 operator 数量和 operand 数量到文件的第二行
    static void write_op_oprd_count(FILE* param_fp, const int op_count, const int oprd_count)
    {
        fprintf(param_fp, "%d%s%d%s", op_count, BLANK_SPACE, oprd_count, LINE_FEED);
    }

    // 写入该操作符的参数 params
    static void write_op_params(FILE* param_fp, const Operator* op)
    {
        /*
         * 列子：
         *     假设处理的是："nn.Conv2d convbn2d_0 1 1 0 1 bias=True dilation=(1,1) groups=1
         *     in_channels=3 kernel_size=(7,7) out_channels=64 padding=(3,3) padding_mode=zeros stride=(2,2)
         *     @bias=(64)f32 @weight=(64,3,7,7)f32 $input=0 #0=(1,3,224,224)f32 #1=(1,64,112,112)f32"
         *
         *     那么他的属性就会是 bias=True dilation=(1,1) groups=1
         *     in_channels=3 kernel_size=(7,7) out_channels=64 padding=(3,3) padding_mode=zeros stride=(2,2)
         */
        for (const auto& [param_name,param_value] : op->params)
        {
            // eg. 输出param_name bias=
            fprintf(param_fp, "%s%s=", BLANK_SPACE, param_name.c_str());
            // 继续输出后面 param_value True
            const Parameter parameter = param_value;
            std::string param_value_string = Parameter::encode_to_string(param_value);
            fprintf(param_fp, "%s", param_value_string.c_str());
            // 综合起来就是输出 " bias=True"，当然这只是众多案例中的一种
        }
    }

    // 写入属性以及其 shape
    static void write_op_attributes(FILE* param_fp, const Operator* op, StoreZipWriter& szw)
    {
        /**
         * eg. @bias=(64)f32 @weight=(64,3,7,7)f32
         */
        for (const auto& [attr_name,attr_value] : op->attrs)
        {
            // 处理有关 @ 的操作符（operator）属性，比如说 bias, weight
            // 输入：'' + ' bias=' => ' @bias='
            fprintf(param_fp, "%s@%s=", BLANK_SPACE, attr_name.c_str());
            const Attribute& attribute = attr_value;
            // ' @bias=' + '(' => ' @bias=('
            fprintf(param_fp, "(");
            // 最后一个留给 ')',主要是防止最后出现多的空格
            for (int i = 0; i < static_cast<int>(attribute.shape.size()) - 1; ++i)
            {
                // ' @bias=(' + '64' => ' @bias=(64'
                fprintf(param_fp, "%d,", attribute.shape[i]);
            }
            if (!attribute.shape.empty())
            {
                // 写入最后一个 dim 的值
                fprintf(param_fp, "%d", attribute.shape.back());
            }
            // ' @bias=(64' + ')' => ' @bias=(64)'
            fprintf(param_fp, ")");
            // ' @bias=(64' + ')' => ' @bias=(64)f32'
            fprintf(param_fp, type_to_string(attribute.type));
            // 将参数写到参数文件里面（zip）
            std::string filename = op->name + "." + op->name;
            szw.write_file(filename, attribute.data.data(), attribute.data.size());
        }
    }

    // 写入 op 的输入 params
    static void write_op_inputs(FILE* param_fp, const Operator* op)
    {
        /*
         * 例子：$input=3 #3=(1,64,56,56)f32
         */
        if (op->input_names.size() == op->inputs.size())
        {
            // 写入输入 params，一般来讲，多输入的比较少见
            // '' + ' $input=3'
            for (size_t i = 0; i < op->input_names.size(); ++i)
            {
                if (op->input_names[i].empty()) continue;
                const Operand* operand = op->inputs[i];
                fprintf(param_fp, "%s$%s=%s", BLANK_SPACE, op->input_names[i].c_str(), operand->name.c_str());
            }
            // ' $input=3' => ' $input=3 #3='
            for (const Operand* operand : op->inputs)
            {
                if (operand->shape.empty()) continue;
                // ' $input=3' => ' $input=3 #3='
                fprintf(param_fp, "%s#%s=", BLANK_SPACE, operand->name.c_str());

                // 处理 (1,3,3,9) 这种
                fprintf(param_fp, "(");
                // ' $input=3 #3=' => ' $input=3 #3=(1,64,64,'
                for (int i = 0; i < static_cast<int>(operand->shape.size()) - 1; ++i)
                {
                    // TODO: 分析这个情况
                    // 暂时不知道这是个什么情况
                    if (operand->shape[i] == -1) fprintf(param_fp, "?,");
                    // ' $input=3 #3=' => ' $input=3 #3=(1,64,64,'
                    else
                    {
                        fprintf(param_fp, "%d,", operand->shape[i]);
                    }
                }
                // ' $input=3 #3=' => ' $input=3 #3=(1,64,64,56'
                if (!operand->shape.empty())
                {
                    if (operand->shape[operand->shape.size() - 1] == -1)
                    {
                        fprintf(param_fp, "?");
                    }
                    else
                    {
                        fprintf(param_fp, "%d", operand->shape[operand->shape.size() - 1]);
                    }
                }
                // ' $input=3 #3=' => ' $input=3 #3=(1,64,64,56)'
                fprintf(param_fp, ")");
                // ' $input=3 #3=' => ' $input=3 #3=(1,64,64,56)f32'
                fprintf(param_fp, type_to_string(operand->type));
            }
            //
        }
    }

    // 写入 op 的输入 params，由于输入参数搞定之后只剩下输出参数，这里就没有整啥名字之类的
    static void write_op_outputs(FILE* param_fp, const Operator* op)
    {
        // 基本情况和上面输入的情况一模一样，这里就不做过多的分析了
        for (const Operand* operand : op->outputs)
        {
            if (operand->shape.empty()) continue;
            fprintf(param_fp, " #%s=", operand->name.c_str());

            // 处理(1,3,3,9)这种情况
            fprintf(param_fp, "(");
            for (int i = 0; i < static_cast<int>(operand->shape.size()) - 1; ++i)
            {
                // 暂时未知的情况
                // TODO: 分析
                if (operand->shape[i] == -1)
                {
                    fprintf(param_fp, "?,");
                }
                else
                {
                    fprintf(param_fp, "%d,", operand->shape[i]);
                }
            }
            if (!operand->shape.empty())
            {
                if (operand->shape[operand->shape.size() - 1] == -1) fprintf(param_fp, "?");
                else
                {
                    fprintf(param_fp, "%d", operand->shape[operand->shape.size() - 1]);
                }
            }
            fprintf(param_fp, ")");
            // 写入 type 的字符串形式
            fprintf(param_fp, type_to_string(operand->type));
        }
    }

    // 写入 param 中某一行的
    // 输入名、输出名、输入操作符（operator）的数量（用于解析后面的参数）、输出操作符（operator）的数量
    static void write_param_line(FILE* param_fp, const Operator* op, StoreZipWriter& szw)
    {
        /*
         * 以下面三行为例子
         * pnnx.Input pnnx_input_0 0 1 0 #0=(1,3,224,224)f32
         * nn.Conv2d convbn2d_0 1 1 0 1 bias=True dilation=(1,1) groups=1 in_channels=3 kernel_size=(7,7) out_channels=64 padding=(3,3) padding_mode=zeros stride=(2,2) @bias=(64)f32 @weight=(64,3,7,7)f32 $input=0 #0=(1,3,224,224)f32 #1=(1,64,112,112)f32
         * pnnx.Output pnnx_output_0 1 0 49 #49=(1,1000)f32
         */

        // 写入的格式如下
        // 'op_type op_name op_input_oprd_num op_output_oprd_num'
        // 例如:
        //  pnnx.Input pnnx_input_0 0 1
        //  nn.Conv2d convbn2d_0 1 1
        //  pnnx.Output pnnx_output_0 1 0
        fprintf(param_fp, "%-24s%s%-24s%s%d%s%d", op->type.c_str(), BLANK_SPACE,
                op->name.c_str(), BLANK_SPACE, static_cast<int>(op->inputs.size()), BLANK_SPACE,
                static_cast<int>(op->outputs.size()));
        // 写入输入操作数（operand）的名字
        // 然后栗子变成了下面这种
        //  'pnnx.Input pnnx_input_0 0 1' + '' => 'pnnx.Input pnnx_input_0 0 1'
        //  'nn.Conv2d convbn2d_0 1 1' + ' 0' => 'nn.Conv2d convbn2d_0 1 1 0'
        //  'pnnx.Output pnnx_output_0 1 0' + ' 49' => 'pnnx.Output pnnx_output_0 1 0 49'
        for (const Operand* operand : op->inputs)
        {
            fprintf(param_fp, "%s%s", BLANK_SPACE, operand->name.c_str());
        }
        // 写入输出操作数（operand）的**名字**
        // 然后栗子变成了下面这种
        //  'pnnx.Input pnnx_input_0 0 1' + ' 0' => 'pnnx.Input pnnx_input_0 0 1'
        //  'nn.Conv2d convbn2d_0 1 1 0' + ' 1' => 'nn.Conv2d convbn2d_0 1 1 0 1'
        //  'pnnx.Output pnnx_output_0 1 0 49' + '' => 'pnnx.Output pnnx_output_0 1 0 49'
        for (const Operand* operand : op->outputs)
        {
            fprintf(param_fp, "%s%s", BLANK_SPACE, operand->name.c_str());
        }

        // 写入操作符的参数信息
        // 然后栗子变成了下面这种
        //  'pnnx.Input pnnx_input_0 0 1' + '' => 'pnnx.Input pnnx_input_0 0 1'
        //  'nn.Conv2d convbn2d_0 1 1 0 1' + ' bias=True dilation=(1,1) groups=1 in_channels=3 kernel_size=(7,7)
        //      out_channels=64 padding=(3,3) padding_mode=zeros stride=(2,2)'
        //      => 'nn.Conv2d convbn2d_0 1 1 0 1  bias=True dilation=(1,1) groups=1 in_channels=3 kernel_size=(7,7)
        //      out_channels=64 padding=(3,3) padding_mode=zeros stride=(2,2)'
        //  'pnnx.Output pnnx_output_0 1 0 49' + '' => 'pnnx.Output pnnx_output_0 1 0 49'
        write_op_params(param_fp, op);

        // 写入操作符的参数信息
        // 然后栗子变成了下面这种
        //  'pnnx.Input pnnx_input_0 0 1' + '' => 'pnnx.Input pnnx_input_0 0 1'
        //  胜率额前面的，中间这个例子简单些就是 ' ' => ' @bias=(64)f32'
        //  'pnnx.Output pnnx_output_0 1 0 49' + '' => 'pnnx.Output pnnx_output_0 1 0 49'
        write_op_attributes(param_fp, op, szw);
        // $input=3 #3=(1,64,56,56)f32 #3=(1,64,56,56)f32 #4=(1,64,56,56)f32
        // ' $input=3 #3=(1,64,56,56)f32 #3=(1,64,56,56)f32'
        write_op_inputs(param_fp, op);
        // ' #4=(1,64,56,56)f32'
        write_op_outputs(param_fp, op);
        fprintf(param_fp, "%s", LINE_FEED);
    }
}

// 静态辅助函数，辅解析 param => 处理 graph::load 的辅助函数
namespace cnnx
{
}

// graph 类初始化构造和赋值的实现部分
namespace cnnx
{
    // 默认构造函数
    Graph::Graph() = default;
    // 析构函数
    Graph::~Graph()
    {
        for (const auto op : this->operators)
        {
            delete op;
        }
        for (const auto operand : this->operands)
        {
            delete operand;
        }
        this->operators.clear();
        this->operands.clear();
    }

    // 赋值构造函数，这里什么也不做
    Graph::Graph(const Graph& /*rhs*/)
    {
    }

    // 赋值构造函数，直接返回调用者本身的指针
    Graph& Graph::operator=(const Graph& /*rhs*/)
    {
        return *this;
    }
}

// graph 类加载和解析参数的实现部分
namespace cnnx
{
    // TODO: 内存泄漏?
    // 加载参数文件
    int Graph::load(const std::string& param_path, const std::string& bin_path)
    {
        // 文件流，用于以二进制的形式读取文件里面的数据
        std::ifstream ifs(param_path, std::ios::in | std::ios::binary);
        // 打开 param 文件失败
        if (!ifs.good())
        {
            fprintf(stderr, "open file %s failed\n", param_path.c_str());
            return -1;
        }

        StoreZipReader szr{};
        // 打开 bin 文件失败
        if (szr.open(bin_path) != 0)
        {
            fprintf(stderr, "open file %s failed\n", bin_path.c_str());
            return -1;
        }

        // TODO:分析代码
        // param 文件的第一个参数是模型参数的数量
        {
            int magic = 0;
            std::string line;
            std::getline(ifs, line);
            std::istringstream iss(line);
            iss >> magic;
        }
        int operator_count = 0;
        // 获取 param 文件里面的第二行，其数据代表着一个魔数
        {
            int operand_count = 0;
            std::string line;
            std::getline(ifs, line);
            std::istringstream iss(line);
            iss >> operator_count >> operand_count;
        }

        // 逐行解析 param 里面的参数结构
        // 详细的结构信息在之后的文档里面给出
        for (int i = 0; i < operator_count; ++i)
        {
            std::string line;
            std::getline(ifs, line);
            std::istringstream iss(line);

            std::string type;
            std::string name;
            int input_count = 0;
            int output_count = 0;
            iss >> type >> name >> input_count >> output_count;

            // TODO: 分析和注释这个代码
            Operator* op = this->new_operator(type, name);
            for (int j = 0; j < input_count; ++j)
            {
                std::string operand_name;
                iss >> operand_name;

                Operand* operand = this->get_operand(operand_name);
                operand->consumers.push_back(op);
                op->inputs.push_back(operand);
            }
            // TODO: 分析和注释这个代码
            for (int j = 0; j < output_count; ++j)
            {
                std::string operand_name;
                iss >> operand_name;

                Operand* operand = this->new_operand(operand_name);
                operand->producer = op;
                op->outputs.push_back(operand);
            }

            // key = value
            while (!iss.eof())
            {
                std::string param;
                iss >> param;

                std::string key;
                std::string value;
                std::istringstream pss(param);
                std::getline(pss, key, '=');
                std::getline(pss, value);

                // 解析 param
                if (key[0] == '@')
                {
                    // attribute
                    load_attribute(op, key.substr(1), value, szr);
                }
                else if (key[0] == '$')
                {
                    // operand input key
                    load_input_key(op, key.substr(1), value);
                }
                else if (key[0] == '#')
                {
                    // operand shape
                    load_shape(op, key.substr(1), value);
                }
                else
                {
                    // parameter
                    load_parameter(op, key, value);
                }
            }
        }
        return 0;
    }

    // TODO: 分析这个代码
    // 保存参数文件
    int Graph::save(const std::string& param_path, const std::string& bin_path) const
    {
        FILE* param_fp = fopen(param_path.c_str(), "wb");
        if (!param_fp)
        {
            fprintf(stderr, "fopen file %s failed\n", param_path.c_str());
            return -1;
        }

        StoreZipWriter szw;
        if (szw.open(bin_path) != 0)
        {
            fprintf(stderr, "open file %s failed\n", bin_path.c_str());
            return -1;
        }

        // 写入魔数到模型 param 文件当中
        write_magic_number(param_fp);

        // 写入 op count 和 oprand count
        write_op_oprd_count(param_fp, static_cast<int>(this->operators.size()),
                            static_cast<int>(this->operands.size()));
        // 处理下面的 params
        for (const Operator* op : this->operators)
        {
            write_param_line(param_fp, op, szw);
        }

        // 关闭文件流
        fclose(param_fp);
        return 0;
    }

    // TODO: 分析这个代码
    // 处理参数文件
    int Graph::parse(const std::string& param)
    {
        std::istringstream param_iss(param);
        if (!param_iss.good())
        {
            fprintf(stderr, "open param file %s failled\n", param.c_str());
            return -1;
        }

        // 处理魔数
        {
            int magic = 0;
            std::string line;
            std::getline(param_iss, line);
            std::istringstream line_iss(line);

            line_iss >> magic;
        }

        int operator_count = 0;
        {
            int operand_count = 0;
            std::string line;
            std::getline(param_iss, line);
            std::istringstream line_iss(line);

            line_iss >> operator_count >> operand_count;
        }

        for (int i = 0; i < operator_count; ++i)
        {
            std::string line;
            std::getline(param_iss, line);
            std::istringstream line_iss(line);

            std::string type;
            std::string name;
            int input_count = 0;
            int output_count = 0;
            line_iss >> type >> name >> input_count >> output_count;

            Operator* op = new_operator(type, name);
            for (int j = 0; j < input_count; ++j)
            {
                std::string operand_name;
                line_iss >> operand_name;

                Operand* operand = get_operand(operand_name);
                operand->consumers.push_back(op);
                op->inputs.push_back(operand);
            }

            for (int j = 0; j < output_count; ++j)
            {
                std::string operand_name;
                line_iss >> operand_name;

                Operand* operand = new_operand(operand_name);
                operand->producer = op;
                op->outputs.push_back(operand);
            }

            while (!line_iss.eof())
            {
                std::string line_param;
                line_iss >> line_param;

                std::string key;
                std::string value;
                std::istringstream line_param_iss(line_param);
                std::getline(line_param_iss, key, '=');
                std::getline(line_param_iss, value);

                if (key[0] == '@')
                {
                    // attribute
                    // load_attribute(op, key.substr(1), value, szr);
                    op->attrs[key.substr(1)] = Attribute();
                    Attribute& attr = op->attrs[key.substr(1)];

                    attr.type = 0;
                    if (value.empty()) continue;
                    if (value[0] == '%')
                    {
                        // @data=%op1.data
                        attr.data = std::vector<char>(value.begin() + 1, value.end());
                    }
                    if (value[0] == '(')
                    {
                        // @data=(1,%c,?,4)f32

                        //type
                        std::string typestr = value.substr(value.find_last_of(')') + 1);
                        attr.type = string_to_type(typestr.c_str());

                        // shape
                        std::string lc = value.substr(1, value.find_last_of(')') - 1);
                        std::istringstream lcss(lc);

                        attr.shape.clear();
                        while (!lcss.eof())
                        {
                            std::string element;
                            std::getline(lcss, element, ',');

                            if (element == "?")
                            {
                                attr.shape.push_back(-1);
                            }
                            else if (element[0] == '%')
                            {
                                // encode %abc as symbolic tag
                                attr.shape.push_back(-233);
                                int index = static_cast<int>(attr.shape.size() - 1);
                                std::string element_key = element.substr(1);
                                attr.params[std::string("__shape__") + std::to_string(index)] = Parameter(element_key);
                            }
                            else
                            {
                                int element_int = std::stoi(element);
                                attr.shape.push_back(element_int);
                            }
                        }
                    }
                }
                else if (key[0] == '$')
                {
                    // operand input key
                    load_input_key(op, key.substr(1), value);
                }
                else if (key[0] == '#')
                {
                    // operand shape
                    load_shape(op, key.substr(1), value);
                }
                else
                {
                    // parameter
                    load_parameter(op, key, value);
                }
            }
        }
        return 0;
    }
}

// 创建运算数和运算符
// TODO: 内存泄漏?
namespace cnnx
{
    // 创建一个新的运算符,用函数参数赋值这个运算符的 type 和 name
    // 返回运算符的指针
    Operator* Graph::new_operator(const std::string& type, const std::string& name)
    {
        auto* op = new Operator;
        op->type = type;
        op->name = name;
        this->operators.push_back(op);
        return op;
    }

    // 在 current_operator 前创建一个新的运算符,用函数参数赋值这个运算符的 type 和 name
    // 返回运算符的指针
    Operator* Graph::new_operator_before(const std::string& type, const std::string& name,
                                         const Operator* current_operator)
    {
        auto* op = new Operator;
        op->type = type;
        op->name = name;
        const auto pos = std::find(this->operators.begin(), this->operators.end(), current_operator);
        this->operators.insert(pos, op);
        return op;
    }

    // 在 current_operator 后创建一个新的运算符,用函数参数赋值这个运算符的 type 和 name
    // 返回运算符的指针
    Operator* Graph::new_operator_after(const std::string& type, const std::string& name,
                                        const Operator* current_operator)
    {
        auto* op = new Operator;
        op->type = type;
        op->name = name;
        const auto pos = std::find(this->operators.begin(), this->operators.end(), current_operator) + 1;
        this->operators.insert(pos, op);
        return op;
    }

    // 创建一个新的运算数,用函数参数赋值这个运算数的 type 和 name
    // 返回运算数的指针
    Operand* Graph::new_operand(const std::string& name)
    {
        auto* opd = new Operand;
        opd->name = name;
        this->operands.push_back(opd);
        return opd;
    }

    // 遍历获取名为 name 的 operand 对象
    Operand* Graph::get_operand(const std::string& name)
    {
        for (const auto opd : this->operands)
        {
            if (opd->name == name)
            {
                return opd;
            }
        }
        return nullptr;
    }

    // 遍历获取名为 name 的 operand 对象
    const Operand* Graph::get_operand(const std::string& name) const
    {
        for (const auto opd : this->operands)
        {
            if (opd->name == name)
            {
                return opd;
            }
        }
        return nullptr;
    }
}
