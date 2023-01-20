//测试块的初始化
#include "index_handle.h"
#include "file_op.h"
#include "common.h"
#include <sstream>
using namespace myProject;
using namespace std;

static const int debug = 1;

//内存映射参数
const static largefile::MMapOption mmap_option = {1024000,4096,4096};
//主块文件大小
const static uint32_t main_blocksize = 1024*1024*64;
//哈希桶的大小
const static uint32_t bucket_size = 1000;

static int32_t block_id = 1;

int main(void)
{
    string mainblock_path = "";
    string index_path = "";
    int32_t ret = largefile::TFS_SUCCESS;

    cout<<"please input your block id:"<<endl;
    cin>>block_id;
    if(block_id < 1)
    {
        cerr<<"invalid blockid!!"<<endl;
        return -1;
    }
    
    

    //1.加载索引文件
    largefile::IndexHandle* index_handle = new largefile::IndexHandle(".",block_id);
    if(debug)
    {
        fprintf(stdout,"load index file...\n");
    }
    ret = index_handle->load(block_id,bucket_size,mmap_option);
    if(ret != largefile::TFS_SUCCESS)
    {
       fprintf(stderr,"load index %d failed.\n",block_id); 
       delete index_handle;
       return -2;
    }

    //2.读取文件的metaInfo
    uint64_t file_id = 0;
    cout<<"please input your file id:"<<endl;
    cin>>file_id;
    if(file_id < 1)
    {
        cerr<<"invalid file id!!"<<endl;
        return -2;
    }
    largefile::MetaInfo meta;
    ret = index_handle->read_segment_meta(file_id,meta);
    if(ret!=largefile::TFS_SUCCESS)
    {
        fprintf(stderr,"read_segment_meta failed.file id:%lu,reason %d\n",file_id,ret);
        return -3;
    }
    
    //3.根据metaInfo读取文件
    //2.将文件写入到主块文件
    std::stringstream tmp_streams;
    tmp_streams << "."<<largefile::MAINBLOCK_DIR_PREFIX<<block_id;
    tmp_streams >> mainblock_path;
    //创建主块操作对象
    largefile::FileOperation* mainblock = new largefile::FileOperation(mainblock_path,O_RDWR|O_LARGEFILE);
    char* buffer = new char(meta.get_size()+1);
    ret = mainblock->pread_file(buffer,meta.get_size(),meta.get_offset());
    if(ret!=largefile::TFS_SUCCESS)
    {
        fprintf(stderr,"read to main block failed.ret:%d,reason:%s\n",ret,strerror(errno));
        mainblock->close_file();
        delete mainblock;
        delete index_handle;
        return -3;
    }
    //如果成功了就把内容进行打印
    buffer[meta.get_size()]='\0';
    fprintf(stdout,"read size %d\n,%s\n",meta.get_size(),buffer);

    mainblock->close_file();
    delete mainblock;
    delete index_handle;
    return 0;
}