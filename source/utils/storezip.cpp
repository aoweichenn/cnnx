//
// Created by aoweichen on 2025/5/29.
//


#include <runtime/cnnx/utils/storezip.hpp>


namespace pnnx
{
#ifdef _MSC_VER
#define PACK(__Declaration__) __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#else
#define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#endif
}

// datastructure
namespace cnnx
{
    PACK(struct local_file_header {
        uint16_t version;
        uint16_t flag;
        uint16_t compression;
        uint16_t last_modify_time;
        uint16_t last_modify_date;
        uint32_t crc32;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint16_t file_name_length;
        uint16_t extra_field_length;
        });

    PACK(struct zip64_extended_extra_field {
        uint64_t uncompressed_size;
        uint64_t compressed_size;
        uint64_t lfh_offset;
        uint32_t disk_number;
        });

    PACK(struct central_directory_file_header {
        uint16_t version_made;
        uint16_t version;
        uint16_t flag;
        uint16_t compression;
        uint16_t last_modify_time;
        uint16_t last_modify_date;
        uint32_t crc32;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint16_t file_name_length;
        uint16_t extra_field_length;
        uint16_t file_comment_length;
        uint16_t start_disk;
        uint16_t internal_file_attrs;
        uint32_t external_file_attrs;
        uint32_t lfh_offset;
        });

    PACK(struct zip64_end_of_central_directory_record {
        uint64_t size_of_eocd64_m12;
        uint16_t version_made_by;
        uint16_t version_min_required;
        uint32_t disk_number;
        uint32_t start_disk;
        uint64_t cd_records;
        uint64_t total_cd_records;
        uint64_t cd_size;
        uint64_t cd_offset;
        });

    PACK(struct zip64_end_of_central_directory_locator {
        uint32_t eocdr64_disk_number;
        uint64_t eocdr63_offset;
        uint32_t disk_count;
        });

    PACK(struct end_of_central_directory_record {
        uint16_t disk_number;
        uint16_t start_disk;
        uint16_t cd_records;
        uint16_t total_cd_records;
        uint32_t cd_size;
        uint32_t cd_offset;
        uint16_t comment_length;
        });
}

// CRC32
namespace cnnx
{
    static uint32_t CRC32_TABLE[256];

    static void CRC32_TABLE_INIT()
    {
        for (int i = 0; i < 256; ++i)
        {
            uint32_t c = i;
            for (int j = 0; j < 8; ++j)
            {
                c & 1 ? c = (c >> 1) ^ 0xedb88320 : c >>= 1;
            }
            CRC32_TABLE[i] = c;
        }
    }

    static uint32_t CRC32(const uint32_t x, const u_char ch)
    {
        return (x >> 8) ^ CRC32_TABLE[(x ^ ch) & 0xff];
    }

    static uint32_t CRC32_BUFFER(const u_char* data, const uint64_t length)
    {
        uint32_t x = 0xffffffff;
        for (uint64_t i = 0; i < length; ++i)
        {
            x = CRC32(x, data[i]);
        }
        return x ^ 0xffffffff;
    }
}

// StoreZipReader
namespace cnnx
{
    StoreZipReader::StoreZipReader()
    {
        this->fp = nullptr;
    }

    StoreZipReader::~StoreZipReader()
    {
        this->close();
    }

    //TODO: 笔记记录新语法，结构化绑定和 range-based
    std::vector<std::string> StoreZipReader::get_names() const
    {
        std::vector<std::string> names;
        for (const auto& [fst, snd] : this->filemetas)
        {
            names.push_back(fst);
        }
        return names;
    }

    //TODO: 笔记记录std::map的使用方法
    uint64_t StoreZipReader::get_file_size(const std::string& name) const
    {
        if (this->filemetas.find(name) == this->filemetas.end())
        {
            fprintf(stderr, "no such file %s\n", name.c_str());
            return 0;
        }
        return this->filemetas.at(name).size;
    }

    //TODO: 笔记分析代码
    int StoreZipReader::read_file(const std::string& name, char* data)
    {
        if (this->filemetas.find(name) == this->filemetas.end())
        {
            fprintf(stderr, "no such file %s\n", name.c_str());
            return -1;
        }

        const uint64_t offset = this->filemetas[name].offset;
        const uint64_t size = this->filemetas[name].size;

        fseek(this->fp, static_cast<long>(offset),SEEK_SET);
        fread(data, size, 1, this->fp);
        return 0;
    }

    //TODO: 笔记分析代码
    void StoreZipReader::close()
    {
        if (!this->fp) return;
        fclose(this->fp);
        this->fp = nullptr;
    }

    //TODO: 笔记分析 c 语言文件操作的代码
    int StoreZipReader::open(const std::string& path)
    {
        // 如果 fp 指针为空，则直接返回，
        // 否则使用fclose关闭这个文件指针，并将这个文件指针置为空在返回
        this->close();

        this->fp = fopen(path.c_str(), "rb");
        if (!this->fp)
        {
            fprintf(stderr, "open failed\n");
        }
        while (!feof(this->fp))
        {
            uint32_t signature;
            // TODO: 详细分析这行代码
            if (const uint32_t nread = fread(reinterpret_cast<char*>(&signature), sizeof(signature), 1, this->fp);
                nread != 1) { break; }
            if (signature == 0x04034b50)
            {
                local_file_header lfh{};
                fread(reinterpret_cast<char*>(&lfh), sizeof(lfh), 1, this->fp);
                // TODO: 详细分析c语言中的位运算
                if (lfh.flag & 0x08)
                {
                    fprintf(stderr, "zip file contains data descriptor, this is not supported yet\n");
                    break;
                }
                // 这里由于目前并未实现任何压缩算法，这里自定义的Reader是不能读含有压缩算法的zip文件的
                // 所以这个最终压缩文件和为被压缩之前的大小是一样的
                if (lfh.compression != 0 || lfh.compressed_size != lfh.uncompressed_size)
                {
                    fprintf(stderr, "not stored zip file %d %d\n", lfh.compressed_size, lfh.uncompressed_size);
                    return -1;
                }

                std::string name{};
                name.resize(lfh.file_name_length);
                fread(reinterpret_cast<char*>(name.data()), name.size(), 1, this->fp);

                uint64_t compressed_size = lfh.compressed_size;

                // 当压缩文件的大小大于 4GB 的时候，需要使用额外的描述来描述相关信息
                if (const uint64_t uncompressed_size = lfh.uncompressed_size; compressed_size == 0xffffffff &&
                    uncompressed_size == 0xffffffff)
                {
                    uint16_t extra_offset{};
                    while (extra_offset < lfh.extra_field_length)
                    {
                        uint16_t extra_id{}, extra_size{};
                        fread(reinterpret_cast<char*>(&extra_id), sizeof(extra_id), 1, this->fp);
                        fread(reinterpret_cast<char*>(&extra_size), sizeof(extra_size), 1, this->fp);
                        if (extra_id != 0x0001)
                        {
                            fseek(this->fp, extra_size - 4,SEEK_CUR);
                            extra_offset += extra_size;
                            continue;
                        }

                        zip64_extended_extra_field zipp64_eef{};
                        fread(reinterpret_cast<char*>(&zipp64_eef), sizeof(zipp64_eef), 1, this->fp);
                        compressed_size = zipp64_eef.compressed_size;
                        // uncompressed_size = zipp64_eef.uncompressed_size;

                        // skip remaining extra field blocks
                        fseek(this->fp,
                              static_cast<long>(lfh.extra_field_length - extra_offset - 4 - sizeof(zipp64_eef)),
                              SEEK_CUR);
                        break;
                    }
                }
                else
                {
                    fseek(this->fp, lfh.extra_field_length,SEEK_CUR);
                }

                StoreZipMetaOfReader fmr{};
                fmr.offset = ftell(this->fp);
                fmr.size = compressed_size;

                this->filemetas[name] = fmr;
                fseek(this->fp, static_cast<long>(compressed_size),SEEK_CUR);
            }
            else if (signature == 0x02014b50)
            {
                central_directory_file_header cdfh{};
                fread(reinterpret_cast<char*>(&cdfh), sizeof(cdfh), 1, this->fp);
                fseek(this->fp, cdfh.file_name_length,SEEK_CUR);
                fseek(this->fp, cdfh.extra_field_length,SEEK_CUR);
                fseek(this->fp, cdfh.file_comment_length,SEEK_CUR);
            }
            else if (signature == 0x06054b50)
            {
                end_of_central_directory_record eocdr{};
                fread(reinterpret_cast<char*>(&eocdr), sizeof(eocdr), 1, this->fp);

                fseek(this->fp, eocdr.comment_length,SEEK_CUR);
            }
            else if (signature == 0x06064b50)
            {
                zip64_end_of_central_directory_record eocdr64{};
                fread(reinterpret_cast<char*>(&eocdr64), sizeof(eocdr64), 1, this->fp);
                fseek(this->fp, static_cast<long>(eocdr64.size_of_eocd64_m12 - 44),SEEK_CUR);
            }
            else if (signature == 0x07064b50)
            {
                zip64_end_of_central_directory_locator eocdl64{};
                fread(reinterpret_cast<char*>(&eocdl64), sizeof(eocdl64), 1, fp);
            }
            else
            {
                fprintf(stderr, "Unsupported signature %x\n", signature);
                return -1;
            }
        }
        return 0;
    }
}

// StoreZipWriter
namespace cnnx
{
    StoreZipWriter::StoreZipWriter()
    {
        this->fp = nullptr;
        CRC32_TABLE_INIT();
    }

    StoreZipWriter::~StoreZipWriter()
    {
        this->close();
    }

    int StoreZipWriter::close()
    {
        if (!this->fp) return -1;

        const long offset1 = ftell(this->fp);
        for (const auto& [name, lfh_offset, crc32, size] : this->filemetas)
        {
            uint32_t signature = 0x02014b50;
            fwrite(reinterpret_cast<char*>(&signature), sizeof(signature), 1, this->fp);

            central_directory_file_header cdfh{
                .version_made = 0, .version = 0, .flag = 0, .compression = 0, .last_modify_time = 0,
                .last_modify_date = 0, .crc32 = crc32, .compressed_size = 0xffffffff,
                .uncompressed_size = 0xffffffff, .file_name_length = static_cast<uint16_t>(name.size()),
                .file_comment_length = 0, .start_disk = 0xffff, .internal_file_attrs = 0, .external_file_attrs = 0,
                .lfh_offset = 0xffffffff
            };

            zip64_extended_extra_field zip64_eef{
                .uncompressed_size = size, .compressed_size = size, .lfh_offset = lfh_offset,
                .disk_number = 0
            };

            uint16_t extra_id = 0x0001;
            uint16_t extra_size = sizeof(zip64_eef);

            cdfh.extra_field_length = sizeof(extra_id) + sizeof(extra_size) + sizeof(zip64_eef);

            fwrite(reinterpret_cast<char*>(&cdfh), sizeof(cdfh), 1, this->fp);
            fwrite(const_cast<char*>(name.c_str()), name.size(), 1, this->fp);
            fwrite(reinterpret_cast<char*>(&extra_id), sizeof(extra_id), 1, this->fp);
            fwrite(reinterpret_cast<char*>(&extra_size), sizeof(extra_size), 1, this->fp);
            fwrite(reinterpret_cast<char*>(&zip64_eef), sizeof(zip64_eef), 1, this->fp);
        }

        const long offset2 = ftell(this->fp);
        {
            uint32_t signature = 0x06064b50;
            fwrite(reinterpret_cast<char*>(&signature), sizeof(signature), 1, this->fp);
            zip64_end_of_central_directory_record eocdr64{
                .size_of_eocd64_m12 = sizeof(eocdr64) - 8, .version_made_by = 0, .version_min_required = 0,
                .disk_number = 0, .start_disk = 0, .cd_records = filemetas.size(), .total_cd_records = filemetas.size(),
                .cd_size = static_cast<uint64_t>(offset2 - offset1), .cd_offset = static_cast<uint64_t>(offset1),
            };
            fwrite(reinterpret_cast<char*>(&eocdr64), sizeof(eocdr64), 1, this->fp);
        }
        {
            uint32_t signature = 0x07064b50;
            fwrite(reinterpret_cast<char*>(&signature), sizeof(signature), 1, this->fp);

            zip64_end_of_central_directory_locator eocdl64{
                .eocdr64_disk_number = 0, .eocdr63_offset = static_cast<uint64_t>(offset2), .disk_count = 1
            };
            fwrite(reinterpret_cast<char*>(&eocdl64), sizeof(eocdl64), 1, this->fp);
        }
        {
            uint32_t signature = 0x06054b50;
            fwrite(reinterpret_cast<char*>(&signature), sizeof(signature), 1, this->fp);

            end_of_central_directory_record eocdr{
                .disk_number = 0xffff, .start_disk = 0xffff, .cd_records = 0xffff, .total_cd_records = 0xffff,
                .cd_size = 0xffffffff, .cd_offset = 0xffffffff, .comment_length = 0
            };

            fwrite(reinterpret_cast<char*>(&eocdr), sizeof(eocdr), 1, this->fp);
        }
        fclose(this->fp);
        this->fp = nullptr;
        return 0;
    }

    int StoreZipWriter::open(const std::string& path)
    {
        this->close();
        this->fp = fopen(path.c_str(), "wb");
        if (!this->fp)
        {
            fprintf(stderr, "open failed\n");
            return -1;
        }
        return 0;
    }

    int StoreZipWriter::write_file(const std::string& name, const char* data, const uint64_t size)
    {
        const long offset = ftell(this->fp);
        uint32_t signature = 0x04034b50;
        fwrite(reinterpret_cast<char*>(&signature), sizeof(signature), 1, this->fp);

        const uint32_t crc32 = CRC32_BUFFER(reinterpret_cast<const u_char*>(data), size);

        local_file_header lfh{
            .version = 0, .flag = 0, .compression = 0, .last_modify_time = 0,
            .last_modify_date = 0, .crc32 = crc32, .compressed_size = 0xffffffff, .uncompressed_size = 0xffffffff,
        };
        lfh.file_name_length = name.size();

        zip64_extended_extra_field zip64_eef{
            .uncompressed_size = size, .compressed_size = size, .lfh_offset = 0, .disk_number = 0
        };
        uint16_t extra_id = 0x0001;
        uint16_t extra_size = sizeof(zip64_eef);

        lfh.extra_field_length = sizeof(extra_id) + sizeof(extra_size) + sizeof(zip64_eef);
        fwrite(reinterpret_cast<char*>(&lfh), sizeof(lfh), 1, this->fp);
        fwrite(const_cast<char*>(name.c_str()), name.size(), 1, this->fp);

        fwrite(reinterpret_cast<char*>(&extra_id), sizeof(extra_id), 1, this->fp);
        fwrite(reinterpret_cast<char*>(&extra_size), sizeof(extra_size), 1, this->fp);
        fwrite(reinterpret_cast<char*>(&zip64_eef), sizeof(zip64_eef), 1, this->fp);

        fwrite(data, size, 1, this->fp);

        StoreZipMetaOfWriter szmow{};
        szmow.name = name;
        szmow.lfh_offset = offset;
        szmow.crc32 = crc32;
        szmow.size = size;

        this->filemetas.push_back(szmow);
        return 0;
    }
}
