//
// Created by aowei on 25-6-20.
//

#include <runtime/cnnx/utils/fp16_converter.hpp>

namespace cnnx
{
    // 由于 cpp 内部标准并不支持 fp16，所以需要手动将 fp32 转为 fp16
    unsigned short float32_to_float16(const float fp32)
    {
        // ISO 标准下 fp32 的位比例如下：1 : 8 : 23
        // 将 uint32 和 fp32 做成联合数据结构，就可以用访问 uint32 的方式访问 fp32
        const union
        {
            // TODO:替换为 uint32_t
            unsigned int uint32;
            float fp32;
        } temporary{.fp32 = fp32};
        // 0x80_00_00_00 => b10000000_00000000_00000000_00000000 获取符号位
        // 右移 31 位，舍去左边的 31 位，只留下符号位，最后数值位 b0000_000x，sign是一字节
        const unsigned short sign = (temporary.uint32 & 0x80000000) >> 31;
        // 0x7f_80_00_00 => b0111_1111__1000_0000__0000_0000__0000_0000
        // 右移 23 位，并且通过类型和取与运算忽略掉符号位
        const unsigned short exponent = (temporary.uint32 & 0x7f800000) >> 23;
        // b0000_0000__0111_1111__1111_1111__1111_1111 => 0x00_7f_ff_ff
        // 这里只有经过位运算取到的 23 位尾数
        const unsigned int significand = (temporary.uint32 & 0x007fffff);


        // 在 64 位平台上，short 的大小是 2 个字节
        unsigned short fp16{};
        if (exponent == 0)
        {
            // 0 值和 非规格值的情况，这里处理的话直接不处理（不管）小数位，全当 0 处理
            // 当指数位为 0 时，由于精度问题这里就直接当做正负 0 来处理
            // 精度问题就直接舍弃掉非规格数的小数位
            fp16 = (sign << 15) | (0x00 << 10) | 0x00;
        }
        else if (exponent == 0xff)
        {
            // 这里处理无穷大和 NAN 的情况
            // fp32正常无穷大（尾数=0）	 fp16(sign) 0x7c00（+∞）
            // fp32NaN（尾数≠0）	fp16(sign) 0x7e00（尾数=0x200）
            // 这里所有的 NAN 都统一用 0x200 尾数表示，不探究细节，这种表示就是说不对 NAN 做处理
            // 这里面指数位 255，表示无穷大，在FP16中，指数全 1 表示特殊值（无穷大或NaN）
            // fp16 只有 5 位指数 1 : 5 : 10
            fp16 = (sign << 15) | (0x1f << 10) | (significand ? 0x200 : 0x00);
        }
        else
        {
            // 常规情况下当 new_exp 数值超过了 fp16 的精度的时候直接当做无穷大处理
            if (const auto new_exp = static_cast<short>(exponent - 127 + 15); new_exp >= 31)
            {
                fp16 = (sign << 15) | (0x1f << 10) | 0x00;
            }
            // 常规情况下当 new_exp 数值超过了 fp16 的精度的时候直接当做 0 处理
            else if (new_exp <= 0)
            {
                fp16 = (sign << 15) | (0x00 << 10) | 0x00;
            }
            // 在 fp16 精度范围之内
            else
            {
                fp16 = (sign << 15) | (new_exp << 10) | (significand >> 13);
            }
        }
        return fp16;
    }

    // fp16 转为 fp32
    float float16_to_float32(const unsigned short fp16)
    {
        // 获取 fp16 符号位
        // 0x8000 => b1000_0000_0000_0000
        unsigned short sign = (fp16 & 0x8000) >> 15;
        // 获取 fp16 指数位
        // 0x7ff => b0111_1100_0000_0000
        unsigned short exponent = (fp16 & 0x7c00) >> 10;
        // 获取 fp16 小数位
        // 0x3ff => b0000_0011_1111_11111
        unsigned int significand = fp16 & 0x3ff;

        // fp32 1 :8 : 23
        union
        {
            unsigned int uint32;
            float fp32;
        } temporary{};
        if (exponent == 0)
        {
            if (significand == 0)
            {
                temporary.uint32 = sign << 31;
            }
            else
            {
                exponent = 0;
                // 检测尾数第9位是否为0
                while ((significand & 0x200) == 0)
                {
                    // 左移尾数，对齐首位1
                    significand <<= 1;
                    // 记录左移次数
                    exponent += 1;
                }
            }
            significand <<= 1;
            significand &= 0x3ff;
            temporary.uint32 = (sign << 31) | ((-exponent + 127 - 15) << 23) | (significand << 13);
        }
        else if (exponent == 0x1f)
        {
            // fp16 的无穷大直接也映射到 fp32 的无穷大
            temporary.uint32 = (sign << 31) | (0xff << 23) | (significand << 13);
        }
        else
        {
            // 普通情况
            temporary.uint32 = (sign << 31) | ((exponent + 127 - 15) << 23) | (significand << 13);
        }
        return temporary.fp32;
    }
}
