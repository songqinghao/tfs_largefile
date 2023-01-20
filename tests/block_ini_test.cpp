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
    
    std::stringstream tmp_streams;
    tmp_streams << "."<<largefile::MAINBLOCK_DIR_PREFIX<<block_id;
    tmp_streams >> mainblock_path;
    cout<<mainblock_path<<endl;
    //创建主块操作对象
    largefile::FileOperation* mainblock = new largefile::FileOperation(mainblock_path,O_RDWR|O_CREAT|O_LARGEFILE);
    

    //1.生成索引文件
    largefile::IndexHandle* index_handle = new largefile::IndexHandle(".",block_id);
    if(debug)
    {
        fprintf(stdout,"init index file...\n");
    }
    ret = index_handle->create(block_id,bucket_size,mmap_option);
    if(ret != largefile::TFS_SUCCESS)
    {
       fprintf(stderr,"create index %d failed.\n",block_id); 
       delete index_handle;
       return -3;
    }

    //2.生成主块文件
    //创建主块文件（调整为设置的指定大小）
    ret = mainblock->ftruncate_file(main_blocksize);
    
    if(ret!=largefile::TFS_SUCCESS)
    {
        fprintf(stderr,"create main block %d failed，reason %s\n",block_id,strerror(errno));
        delete mainblock;
        //将索引文件也进行删除
        index_handle->remove(block_id);
        return -2;
    }
    //3.其他操作
    mainblock->close_file();
    index_handle->flush();

    delete mainblock;
    delete index_handle;
    return 0;



}