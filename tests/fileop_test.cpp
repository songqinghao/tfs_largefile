#include "../file_op.h"
#include "../common.h"
#define DATASIZE 64
using namespace myProject;
using namespace std;

int main(void)
{
    const char* filename = "file_op.txt";
    largefile::FileOperation* fileOp = new largefile::FileOperation(filename,O_CREAT|O_RDWR|O_LARGEFILE);

    int fd = fileOp->open_file(); 
    if(fd < 0)
    {
        fprintf(stderr,"open file %s faild，reason:%s\n",filename,strerror(-fd));
        return -1;
    }
    char buffer[DATASIZE+1];
    memset(buffer,'8',DATASIZE);
    //将数据写入到文件中，偏移为1024,数据大小为DATASIZE
    int ret = fileOp->pwrite_file(buffer,DATASIZE,2*DATASIZE);

    if(ret < 0)
    {
        if(ret == largefile::EXIT_DISK_OPER_INCOMPLETE)
        {
            fprintf(stderr,"pwrite file write length is less than required!\n");
        }else{
            fprintf(stderr,"pwrite file %s faild. reason:%s\n",filename,strerror(-ret));
        }
        return -1;
    }

    fprintf(stdout,"pwrite file %s success!\n",filename);
    
    memset(buffer,0,DATASIZE);
    ret = fileOp->pread_file(buffer,DATASIZE,2*DATASIZE);
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
        buffer[DATASIZE] = '\0';
        fprintf(stdout,"pread file %s success!\n",filename);
        fprintf(stdout,"data is %s\n",buffer);
    }

    memset(buffer,'9',DATASIZE);
    ret = fileOp->write_file(buffer,DATASIZE);
    if(ret < 0)
    {
        if(ret == largefile::EXIT_DISK_OPER_INCOMPLETE)
        {
            fprintf(stderr,"write file write length is less than required!\n");
        }else{
            fprintf(stderr,"write file %s faild. reason:%s\n",filename,strerror(-ret));
        }
        return -1;
    }
    
    //关闭文件
    fileOp->close_file();
    //删除文件
    fileOp->unlink_file();

    return 0;
}