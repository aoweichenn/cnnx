//
// Created by aowei on 25-6-19.
//

#ifndef RUNTIME_CNNX_UTILS_FP16_CONVERTER_HPP
#define RUNTIME_CNNX_UTILS_FP16_CONVERTER_HPP

namespace cnnx
{
    unsigned short float32_to_float16(float fp32);

    float float16_to_float32(unsigned short fp16);
}
#endif //RUNTIME_CNNX_UTILS_FP16_CONVERTER_HPP
