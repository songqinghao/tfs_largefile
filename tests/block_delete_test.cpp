/*************************************
**************删除文件测试*************
**************************************/
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

    //2.删除指定文件的metaInfo
    uint64_t file_id = 0;
    cout<<"please input your file id:"<<endl;
    cin>>file_id;
    if(file_id < 1)
    {
        cerr<<"invalid file id!!"<<endl;
        return -2;
    }
    ret = index_handle->delete_segment_meta(file_id);
    if(ret!=largefile::TFS_SUCCESS)
    {
        fprintf(stderr,"delete index file failed.file_id:%lu,ret:%d\n",file_id,ret);
        return -2;
    }
    ret = index_handle->flush();
    if(ret!=largefile::TFS_SUCCESS)
    {
        fprintf(stderr,"flush mainblock %d failed %d failed.file_no:%lu\n",block_id,file_id);
    }
    fprintf(stdout,"delete successfully!\n");
    delete index_handle;
    return 0;
}