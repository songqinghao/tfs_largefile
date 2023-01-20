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
                //删除index文件：ummap以及unlink index file 
                int remove(const uint32_t logic_block_id);
                //进行一些同步操作
                int flush();
                IndexHeader* index_header()
                {
                    //拿到映射内存的首地址
                    return reinterpret_cast<IndexHeader*>(file_op_->get_map_data());
                }
                int update_block_info(const OperType oper_type,const uint32_t modify_size);

                //拿到块信息
                BlockInfo* block_info()
                {
                    return reinterpret_cast<BlockInfo*> (file_op_->get_map_data());
                }

                int32_t bucket_size() const
                {
                    return reinterpret_cast<IndexHeader*> (file_op_->get_map_data())->bucket_size_;
                }

                //拿到主块文件偏移
                int32_t get_block_data_offset()
                {
                    return index_header()->data_file_offset_;
                }
                //改变
                void commit_block_data_offset(const int file_size)
                {
                    reinterpret_cast<IndexHeader*>(file_op_->get_map_data())->data_file_offset_+=file_size;
                }
                //写metaInfo，传入key值以及meta
                int write_segment_meta(const uint64_t key,MetaInfo& meta);
                int32_t read_segment_meta(const uint64_t key,MetaInfo& meta);
                //hash查找
                int hash_find(const uint64_t key,int32_t& current_offset,int32_t& previous_offset);
                //返回桶数组的首节点
                int32_t* bucket_slot()
                {
                    return reinterpret_cast<int32_t*>(reinterpret_cast<char*>(file_op_->get_map_data()+sizeof(IndexHeader)));
                }
                int32_t hash_insert(const uint32_t key,int32_t& previous_offset,MetaInfo&meta);

                

            private:
                bool hash_compare(const uint64_t left_key,const uint64_t right_key)
                {
                    return left_key==right_key;
                }
                MMapFileOperation*file_op_;
                bool is_load_;//索引文件是否被加载
        };
    }
}

#endif  /*LARGE_FILE_INDEX_HANDLE_H*/