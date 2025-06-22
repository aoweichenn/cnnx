//
// Created by aoweichen on 2025/5/28.
//

#ifndef RUNTIME_CNNX_UTILS_STOREZIP_HPP
#define RUNTIME_CNNX_UTILS_STOREZIP_HPP
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace cnnx
{
    /**
     * @brief ZIP文件读取器元数据结构体（本地文件头核心字段）
     *
     * 本结构体严格遵循ZIP文件格式规范（参照PKWARE APPNOTE 4.3.7），用于存储
     * ZIP归档中每个文件条目的定位信息，这些数据用于快速访问文件内容。
     *
     * @note 数据结构对应关系（基于ZIP64扩展格式）：
     * +----------+---------------+-----------------------------+
     * | 字段名   | 字节范围      | ZIP规范字段                 |
     * +----------+---------------+-----------------------------+
     * | offset   | 0xFFFFFFFF    | 文件内容起始偏移（对应规范：|
     * |          |               | relative offset of file data)|
     * | size     | 0xFFFFFFFF    | 压缩后文件大小（对应规范：  |
     * |          |               | compressed size)            |
     * +----------+---------------+-----------------------------+
     *
     * @attention 关键实现细节：
     * 1. 偏移量计算：offset是从本地文件头(LFH)起始位置到文件内容数据的偏移量，
     *    计算公式：offset = LFH偏移 + 文件头长度(30字节) + 文件名长度 + 扩展字段长度
     * 2. 字节序处理：所有数值字段在读取时需从小端字节序转换成本机字节序
     * 3. 大小限制：当size超过0xFFFFFFFF时，必须使用ZIP64扩展格式解析
     * 4. 验证机制：需通过中央目录记录(CDR)验证偏移量的有效性
     */
    struct StoreZipMetaOfReader
    {
        uint64_t offset; ///< 文件内容数据在ZIP中的绝对偏移量（单位：字节）
        uint64_t size; ///< 压缩后的文件大小（单位：字节），注意与原始大小的区别
    };


    /**
     * @brief ZIP文件写入器元数据结构体（中央目录记录核心字段）
     *
     * 本结构体严格遵循ZIP文件格式规范（参照PKWARE APPNOTE 6.3.6），用于持久化存储
     * 每个文件条目在ZIP归档中的元数据信息，这些数据将最终写入中央目录记录（Central Directory Record）。
     *
     * @note 数据结构对应关系（基于ZIP64扩展格式）：
     * +----------+---------------+-----------------------------+
     * | 字段名   | 字节范围      | ZIP规范字段                 |
     * +----------+---------------+-----------------------------+
     * | name     | 可变长度      | 文件名（对应规范字段：name）|
     * | lfh_offset| 0xFFFFFFFF   | 本地文件头偏移（对应规范：  |
     * |          |               | relative offset of local header)|
     * | crc32    | 0x0000-0x0003 | CRC-32校验值（对应规范：    |
     * |          |               | crc-32)                     |
     * | size     | 0xFFFFFFFF    | 原始文件大小（对应规范：    |
     * |          |               | uncompressed size)          |
     * +----------+---------------+-----------------------------+
     *
     * @attention 关键实现细节：
     * 1. 字节序处理：所有数值字段在写入文件时需转换为小端字节序（little-endian）
     * 2. 编码规范：文件名(name)必须使用UTF-8编码，长度不超过65535字节
     * 3. 偏移量计算：lfh_offset是从ZIP文件起始到本地文件头的绝对偏移量
     * 4. CRC32验证：应在文件内容写入时实时计算，避免内存数据篡改
     * 5. 大小限制：当size超过0xFFFFFFFF时，必须触发ZIP64扩展机制
     */
    struct StoreZipMetaOfWriter
    {
        std::string name; ///< 文件条目名称（UTF-8编码），最大长度不超过65535字节
        uint64_t lfh_offset; ///< 本地文件头(Local File Header)的绝对偏移量（单位：字节）
        uint32_t crc32; ///< 文件内容的CRC32校验码（采用IEEE 802.3多项式计算）
        uint64_t size; ///< 原始未压缩文件大小（单位：字节）
    };
}

namespace cnnx
{
    class StoreZipReader
    {
    public:
        StoreZipReader();

        ~StoreZipReader();

        int open(const std::string& path);

        [[nodiscard]] std::vector<std::string> get_names() const;

        [[nodiscard]] uint64_t get_file_size(const std::string& name) const;

        int read_file(const std::string& name, char* data);

        void close();

    private:
        FILE* fp;
        std::map<std::string, StoreZipMetaOfReader> filemetas;
    };
}

namespace cnnx
{
    class StoreZipWriter
    {
    public:
        StoreZipWriter();

        ~StoreZipWriter();

        int open(const std::string& path);

        int write_file(const std::string& name, const char* data, uint64_t size);

        int close();

    private:
        FILE* fp;
        std::vector<StoreZipMetaOfWriter> filemetas;
    };
}

#endif //RUNTIME_CNNX_UTILS_STOREZIP_HPP
