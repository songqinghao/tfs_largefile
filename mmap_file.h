#ifndef LARGEFILE_MMAPFILE_H_
#define LARGEFILE_MMAPFILE_H_
#include "common.h"
namespace myProject
{
    namespace largefile
    {
        //映射选项
        struct MMapOption
        {
            //最大映射大小
            int32_t max_mmap_size_;
            //第一次映射大小
            int32_t first_mmap_size_;
            //每次增加映射大小所追加的大小
            int32_t per_mmap_size_;
        };
        
        class MMapFile
        {
          public:
              MMapFile();
              explicit MMapFile(const int fd);
              MMapFile(const MMapOption& mmap_option,const int fd);

              ~MMapFile();  
              //同步文件
              bool sync_file();
              //将文件进行映射，同时设置访问权限
              bool map_file(const bool write = false);
              //取得映射到内存数据的首地址
              void* get_data() const;
              //取得映射到内存数据的大小
              int32_t get_size() const;
              
              //解除映射
              bool munmap_file();
              
              //重新映射
              bool remap_file();
          private:
              //调整映射内存的大小
              bool ensure_file_size(const int32_t size);
          private:
              int32_t size_;
              int fd_;
              void* data_;
              struct MMapOption mmap_file_option_;

        };
    }
}


#endif