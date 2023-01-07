#include "../common.h"
#include "../mmap_file.h"


using namespace myProject;
using namespace std;

static const mode_t OPEN_MODE = 0644;
const largefile::MMapOption mmap_option={10240000,4096,4096};

int open_file(string file_name,int open_flags)
{
    int fd = open(file_name.c_str(),open_flags,OPEN_MODE);
    if(fd < 0)
    {
        //返回errno，这样是为了避免errno重置
        return -errno;
    }
    
    return fd;

}
int main(void)
{
    const char* filename = "./mapfile_test.txt";
    //1.打开/创建一个文件，取得文件的句柄
    int fd = open_file(filename,O_RDWR|O_CREAT|O_LARGEFILE);
    
    if(fd<0)
    {
        fprintf(stderr,"open file failed.filename:%s,error desc:%s\n",filename,strerror(-fd));
        return -1;
    }

    printf("fd:%d\n",fd);
    largefile::MMapFile* map_file = new largefile::MMapFile(mmap_option,fd);

    bool is_mapped = map_file->map_file(true);
    //映射成功
    if(is_mapped)
    {
        //追加内存
        map_file->remap_file();
        fprintf(stderr,"data:%p,size:%d\n",map_file->get_data(),map_file->get_size());
        
        //写数据
        memset(map_file->get_data(),'8',map_file->get_size());
        
        //同步文件
        map_file->sync_file();
        map_file->munmap_file();
    }else
    {
        fprintf(stderr,"map file failed");
    }

    close(fd);
    
    return 0;
}