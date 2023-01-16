#ifndef LARGE_FILE_INDEX_HANDLE_H
#define LARGE_FILE_INDEX_HANDLE_H
#include "common.h"
#include "mmap_file_op.h"

namespace myProject
{
    namespace largefile
    {            
        //索引头部类
        struct IndexHeader
        {
            public:
                IndexHeader()
                {
                    memset(this,0,sizeof(IndexHeader));
                }
                BlockInfo block_info_;    //块信息
                int32_t bucket_size_;     //hash桶的大小
                int32_t data_file_offset_;//下个写的数据在主块中的偏移
                int32_t index_file_size_; //索引文件当前偏移
                int32_t free_head_offset_;//可重用节点链表的头部
        };

        class IndexHandle
        {
            public:
                IndexHandle(const std::string& base_path,const uint32_t main_block_);
                ~IndexHandle();
                //创建索引文件
                int create(const uint32_t logic_block_id,const int32_t bucket_size,const MMapOption map_option);
                int load(const uint32_t logic_block_id,const int32_t bucket_size,const MMapOption map_option);
                IndexHeader* index_header()
                {
                    //拿到映射内存的首地址
                    return reinterpret_cast<IndexHeader*>(file_op_->get_map_data());
                }
                //拿到块信息
                BlockInfo* block_info()
                {
                    return reinterpret_cast<BlockInfo*> (file_op_->get_map_data());
                }

                int32_t bucket_size() const
                {
                return reinterpret_cast<IndexHeader*> (file_op_->get_map_data())->bucket_size_;
                }
            private:
                MMapFileOperation*file_op_;
                bool is_load_;//索引文件是否被加载
        };


    }
}

#endif  /*LARGE_FILE_INDEX_HANDLE_H*/