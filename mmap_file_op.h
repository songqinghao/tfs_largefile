#ifndef MMAP_FILE_OP_H
#define MMAP_FILE_OP_H

#include "common.h"
#include "file_op.h"
#include "mmap_file.h"
namespace myProject
{
    namespace largefile
    {
        class MMapFileOperation:public FileOperation
        {
            public:
                /*
                explicit MMapFileOperation(const std::string& file_name, int open_flags = O_CREAT | O_RDWR | O_LARGEFILE) :
                FileOperation(file_name, open_flags), is_mapped_(false), map_file_(NULL)
                {

                }
                */
                explicit MMapFileOperation(const std::string& file_name,const int open_flags = O_CREAT|O_RDWR|O_LARGEFILE):
                FileOperation(file_name,open_flags),map_file_(NULL),is_mapped_(false)
                {
                    
                }

                ~MMapFileOperation()
                {
                    if(map_file_)
                    {
                        delete(map_file_);
                        map_file_ = NULL;
                    }
                }
                //文件映射
                int mmap_file(const MMapOption& mmap_option);
                //文件接触映射
                int munmap_file();


                int pread_file(char* buf,const int32_t size,const int64_t offset);
                int pwrite_file(const char* buf,const int32_t size,const int64_t offset);

                

                //获取映射的数据
                void* get_map_data()const;
                int flush_file();

            private:
                //映射文件
                MMapFile* map_file_;
                //是否映射
                bool is_mapped_;
        };
    }
}





#endif /*MMAP_FILE_OP_H*/