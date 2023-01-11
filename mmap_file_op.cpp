#include "mmap_file_op.h"
#include "common.h"

static int debug = 1;

namespace myProject
{
    namespace largefile
    {
        int MMapFileOperation::mmap_file(const MMapOption& mmap_option)
        {
            if(mmap_option.max_mmap_size_ < mmap_option.first_mmap_size_||mmap_option.max_mmap_size_==0)
            {
                return TFS_ERROR;
            }
            
            int fd = check_file();
            if(fd < 0)
            {
                fprintf(stderr,"MMapFileOperation::mmap_file -check file failed.\n");
                return TFS_ERROR;
            }

            if(!is_mapped_)
            {
                if(map_file_) delete(map_file_);
                
                //构建文件映射对象并映射
                map_file_ = new MMapFile(mmap_option,fd);
                is_mapped_ = map_file_->map_file(true);
                
            }
            if(is_mapped_)
            {
                return TFS_SUCCESS;
            }
            else{
                return TFS_ERROR;
            }

        }

        //文件接触映射
        int MMapFileOperation::munmap_file()
        {
            if(is_mapped_&&map_file_!=NULL)
            {
                //map_file析构的时候会调用munmap进行映射解除
                delete(map_file_);
                is_mapped_ = false;
            }
            return TFS_SUCCESS;
        }

        //获取映射的数据
        void* MMapFileOperation::get_map_data()const
        {
            if(is_mapped_)
            {
                return map_file_->get_data();
            }
            return NULL;
        }

        //读文件
        int MMapFileOperation::pread_file(char* buf,const int32_t size,const int64_t offset)
        {
            //情况1，内存已经完成映射
            //偏移+大小已经超过了映射大小
            if(is_mapped_&&(offset+size) > map_file_->get_size())
            {
                //32位系统下__PRI64_PREFIX为‘l'，64位系统下为‘ll’
                if(debug)fprintf(stderr, "mmap MMapFileOperation::pread_file file pread, size: %d, offset: %" __PRI64_PREFIX "d, map file size: %d. need remap\n",
                                size, offset, map_file_->get_size());
                //内存追加
                map_file_->remap_file();
            }
            //大小合适
            if (is_mapped_ && (offset + size) <= map_file_->get_size())
            {
                memcpy(buf, (char *) map_file_->get_data() + offset, size);
                return TFS_SUCCESS;
            }

            //情况2，内存还没有进行映射或是映射空间不全(那就只能去磁盘读取数据了)
            return FileOperation::pread_file(buf,size,offset);
        }

        int MMapFileOperation::pwrite_file(const char* buf,const int32_t size,const int64_t offset)
        {
            //情况1，内存已经完成映射
            if(is_mapped_&&(offset+size) > map_file_->get_size())
            {
                //32位系统下__PRI64_PREFIX为‘l'，64位系统下为‘ll’
                if(debug)fprintf(stderr, "mmap MMapFileOperation::pwrite_file file pwrite, size: %d, offset: %" __PRI64_PREFIX "d, map file size: %d. need remap\n",
                                size, offset, map_file_->get_size());
                //内存追加
                map_file_->remap_file();
            }
            
            //大小合适
            if (is_mapped_ && (offset + size) <= map_file_->get_size())
            {
                memcpy((char *) map_file_->get_data() + offset,buf, size);
                return TFS_SUCCESS;
            }

            //情况2，内存还没有进行映射或是映射空间不全(那就只能去磁盘写数据了)
            return  FileOperation::pwrite_file(buf,size,offset);

        }

        int MMapFileOperation::flush_file()
        {
            if(is_mapped_)
            {
                if(map_file_->sync_file())
                {
                    return TFS_SUCCESS;
                }
                return TFS_ERROR;
            }

            //内映射内存的话，就采用父类的磁盘同步
            return FileOperation::flush_file();
        }

    }
}