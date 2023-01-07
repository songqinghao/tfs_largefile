#ifndef _FILE_OP_H_
#define _FILE_OP_H_

#include "common.h"
namespace myProject
{
    namespace largefile
    {
        //文件操作类
        class FileOperation
        {
        public:
            FileOperation(const std::string& file_name,const int open_flags = O_RDWR|O_LARGEFILE);
            ~FileOperation();

            int open_file();
            void close_file();

            //将数据写入到磁盘
            int flush_file();

            //删除文件
            int unlink_file();

            //读文件，nbytes为读的大小，offset为偏移量
            int pread_file(char* buf,const int32_t nbytes,const int64_t offset);
            //写文件
            int pwrite_file(const char* buf,const int32_t nbytes,const int64_t offset);

            int write_file(const char* buf,const int32_t nbytes);

            int64_t get_file_size();

            //截断文件长度
            int ftruncate_file(const int64_t length);

            int seek_file(const int64_t offset);

            int get_fd()const
            {
                return fd_;
            }

        protected:
            int fd_;
            int open_flags_;
            char* file_name_;
        protected:
            //0代表8进制，6代表文件拥有者拥有读写权限，后面两个4表示同组成员和其他成员拥有读的权限
            static const mode_t OPEN_MODE = 0644;

            //如果读取磁盘文件失败最多尝试次数
            static const int MAX_DISK_TIMES = 5;
        };
    }
}


#endif /*_FILE_OP_H_*/