#include "../common.h"
#include "../mmap_file_op.h"
#include <iostream>
using namespace std;
using namespace myProject;

const largefile::MMapOption mmap_option={10240000,4096,4096};
int main(void)
{
    int ret = 0;
    const char* filename = "mmap_file_op.txt";
    largefile::MMapFileOperation* mmfo = new largefile::MMapFileOperation(filename);

    int fd = mmfo->open_file();
    if(fd<0)
    {
        fprintf(stderr,"open file failed.filename:%s,error desc:%s\n",filename,strerror(-fd));
        return -1;
    }

    ret = mmfo->mmap_file(mmap_option);
    if(ret==largefile::TFS_ERROR)
    {
        fprintf(stderr,"mmap_file failed，reason：%s",strerror(errno));
        return -2;
    }

    char buffer[128+1];
    memset(buffer,'6',128);
    buffer[127]='\n';
    //测试写
    ret  = mmfo->pwrite_file(buffer,128,8);
    if(ret < 0)
    {
        if(ret == largefile::EXIT_DISK_OPER_INCOMPLETE)
        {
            fprintf(stderr,"write file write length is less than required!\n");
        }else{
            fprintf(stderr,"write file %s faild. reason:%s\n",filename,strerror(-ret));
        }
    }

    //测试读
    memset(buffer,0,128);
    ret = mmfo->pread_file(buffer,128,8);
    if(ret < 0)
    {
        if(ret == largefile::EXIT_DISK_OPER_INCOMPLETE)
        {
            fprintf(stderr,"pread file write length is less than required!\n");
        }else{
            fprintf(stderr,"pread file %s faild. reason:%s\n",filename,strerror(-ret));
        }
        return -1;
    }else
    {
        //设置字符串结束符
        buffer[128] = '\0';
        fprintf(stdout,"pread file %s success!\n",filename);
        fprintf(stdout,"data is %s\n",buffer);
    }

    //同步文件
    ret = mmfo->flush_file();
    if(ret == largefile::TFS_ERROR)
    {
        fprintf(stderr,"flush file %s faild. reason:%s\n",filename,strerror(errno));
    }
    memset(buffer,'8',128);
    buffer[127]='\n';
    ret = mmfo->pwrite_file(buffer,128,4000);
    ret = mmfo->munmap_file();
    mmfo->close_file();


    return 0;
}
