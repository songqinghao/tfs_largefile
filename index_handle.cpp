#include "index_handle.h"
static int debug = 1;
namespace myProject
{
    namespace largefile
    {
        //传入基础路径+主块号
        IndexHandle::IndexHandle(const std::string& base_path,const uint32_t main_block_)
        {
            std::stringstream tmp_stream;
            // /home/sqh/index/1这个路径来看的话，基础路径base_path就是/home/sqh
            tmp_stream<<base_path<<INDEX_DIR_PREFIX<<main_block_;

            std::string index_path;
            tmp_stream >> index_path;

            file_op_ = new MMapFileOperation(index_path,O_CREAT|O_RDWR|O_LARGEFILE);
        }

        IndexHandle::~IndexHandle()
        {
            if(file_op_)
            {
                delete file_op_;
                file_op_ = NULL;
            }
        }

        int IndexHandle::create(const uint32_t logic_block_id,const int32_t bucket_size,const MMapOption map_option)
        {
            int ret = TFS_SUCCESS;
            if(debug)
            printf("index create block: %u index. bucket size: %d, max mmap size: %d, first mmap size: %d, per mmap size: %d\n",
            logic_block_id, bucket_size, map_option.max_mmap_size_, map_option.first_mmap_size_,
            map_option.per_mmap_size_);

            if(is_load_)
            {
                return EXIT_INDEX_ALREADY_LOADED_ERROR;
            }
            //获取索引文件的大小，顺便看有没有
            int64_t file_size = file_op_->get_file_size();

            if(file_size < 0)return TFS_ERROR;
            //空文件，说明还没有创建具体的信息
            else if(file_size==0)
            {
                //索引文件头部的信息
                IndexHeader i_header;
                i_header.block_info_.block_id_=logic_block_id;
                i_header.block_info_.seq_no_ = 1;
                i_header.bucket_size_ = bucket_size;
                i_header.index_file_size_ = sizeof(i_header)+bucket_size*sizeof(int32_t);

                //index header + total buckets
                //进行分配空间
                char* init_data = new char[i_header.index_file_size_];
                memcpy(init_data,&i_header,sizeof(IndexHeader));
                memset(init_data+sizeof(IndexHeader),0,i_header.index_file_size_-sizeof(IndexHeader));

                //将索引文件头部和哈希桶的数据写到index文件中
                ret = file_op_->pwrite_file(init_data,i_header.index_file_size_,0);
                delete[] init_data;
                init_data = NULL;
                if(ret != TFS_SUCCESS)
                {
                    return ret;
                }
                //同步到磁盘
                ret = file_op_->flush_file();
                if(ret != TFS_SUCCESS)
                {
                    return ret;
                }
            }
            else{//索引文件已经存在
                return EXIT_META_UNEXPECT_FOUND_ERROR;
            }

            ret = file_op_->mmap_file(map_option);
            if(ret != TFS_SUCCESS)
            {
                return ret;
            }

            is_load_ = true;
            if(debug)
            {
                printf("init blockid: %u index successful. data file size: %d, index file size: %d, bucket size: %d, free head offset: %d, seqno: %d, size: %d, filecount: %d, del_size: %d, del_file_count: %d version: %d\n",
                        logic_block_id, index_header()->data_file_offset_, index_header()->index_file_size_,
                        index_header()->bucket_size_, index_header()->free_head_offset_, block_info()->seq_no_, block_info()->size_t_,
                        block_info()->file_count_, block_info()->del_size_, block_info()->del_file_count_, block_info()->version_);

            }
            return TFS_SUCCESS;
        }
        int IndexHandle::load(const uint32_t logic_block_id,const int32_t bucket_size,const MMapOption map_option)
        {
            int ret = TFS_SUCCESS;
            //如果已经加载了
            if(is_load_)
            {
                return EXIT_INDEX_ALREADY_LOADED_ERROR;
            }

            //获取索引文件的大小，顺便看有没有
            int64_t file_size = file_op_->get_file_size();
            if(file_size < 0)
            {
                return file_size;
            }else if(file_size==0)//空文件
            {
                return EXIT_INDEX_CORRUPT_ERROR;
            }

            MMapOption tmp_map_option = map_option;
            if(file_size>tmp_map_option.first_mmap_size_&&file_size<=tmp_map_option.max_mmap_size_)
            {
                //将第一次映射文件的大小改为文件大小
                tmp_map_option.first_mmap_size_ = file_size;
            }
            ret = file_op_->mmap_file(tmp_map_option);

            if(ret != TFS_SUCCESS)
            {
                return ret;
            }

            if(this->bucket_size()==0||this->block_info()->block_id_==0)
            {
                fprintf(stderr, "Index corrupt error. blockid: %u, bucket size: %d\n", block_info()->block_id_,
                        this->bucket_size());
                return EXIT_INDEX_CORRUPT_ERROR;
            }

            //检查文件大小
            int32_t index_file_size = sizeof(IndexHeader)+this->bucket_size()*sizeof(int32_t);

            
            if(file_size < index_file_size)
            {
                fprintf(stderr, "Index corrupt error. blockid: %u, bucket size: %d, file size: %ld, index file size: %d\n",
                     block_info()->block_id_, this->bucket_size(), file_size, index_file_size);
                return EXIT_INDEX_CORRUPT_ERROR;
            }

            //比较块id
            if(logic_block_id!=block_info()->block_id_)
            {
                fprintf(stderr, "block id conflict. blockid: %u, index blockid: %u\n", logic_block_id, block_info()->block_id_);
                return EXIT_BLOCKID_CONFLICT_ERROR;
            }

            if(bucket_size!=this->bucket_size())
            {
                fprintf(stderr, "Index configure error. old bucket size: %d, new bucket size: %d\n", this->bucket_size(),
                        bucket_size);
                return EXIT_BUCKET_CONFIGURE_ERROR;
            }
            is_load_ = true;
            printf("load blockid: %u index successful. data file offset: %d, index file size: %d, bucket size: %d, free head offset: %d, seqno: %d, size: %d, filecount: %d, del size: %d, del file count: %d version: %d\n",
                logic_block_id, index_header()->data_file_offset_, index_header()->index_file_size_, this->bucket_size(),
                index_header()->free_head_offset_, block_info()->seq_no_, block_info()->size_t_, block_info()->file_count_,
                block_info()->del_size_, block_info()->del_file_count_, block_info()->version_);
            return TFS_SUCCESS;
        }

        int IndexHandle::remove(const uint32_t logic_block_id)
        {
            //看是否已经加载映射了
            if(is_load_)
            {
                if(logic_block_id != block_info()->block_id_)
                {
                    fprintf(stderr,"block id conflict.block id:%d index block id:%d\n",logic_block_id,block_info()->block_id_);
                    return EXIT_BLOCKID_CONFLICT_ERROR;
                }
            }

            int ret = file_op_->munmap_file();
            if(ret != TFS_SUCCESS)
            {
                return ret;
            }
            //删除文件
            ret = file_op_->unlink_file();
            
        }
        int IndexHandle::flush()
        {
            int ret = file_op_->flush_file();
            if(ret != TFS_SUCCESS)
            {
                fprintf(stderr,"index flush failed!,reason:%s",strerror(errno));
            }
            return ret;
        }
    }
}