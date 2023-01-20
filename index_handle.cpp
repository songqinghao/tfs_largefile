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
        int IndexHandle::update_block_info(const OperType oper_type,const uint32_t modify_size)
        {
            //块ID不能为0
            if(block_info()->block_id_==0)
            {
                return EXIT_BLOCKID_ZERO_ERROR;
            }

            if(oper_type==C_OPER_INSERT)
            {
                ++block_info()->version_;
                ++block_info()->file_count_;
                ++block_info()->seq_no_;
                block_info()->size_t_+=modify_size;
            }
            else if(oper_type==C_OPER_DELETE)
            {
                ++block_info()->version_;
                --block_info()->file_count_;
                block_info()->size_t_-=modify_size;
                ++block_info()->del_file_count_;
                block_info()->del_size_+=modify_size;
            }
            if(debug)
                printf("update block info. blockid: %u, version: %u, file count: %u, size: %u, del file count: %u, del size: %u, seq no: %u, oper type: %d\n",
                    block_info()->block_id_, block_info()->version_, block_info()->file_count_, block_info()->size_t_,
                    block_info()->del_file_count_, block_info()->del_size_, block_info()->seq_no_, oper_type);
            return TFS_SUCCESS;
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
            if(debug)
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

        int32_t IndexHandle::write_segment_meta(const uint64_t key,MetaInfo& meta)
        {
            int32_t current_offset = 0;
            int32_t previous_offset = 0;
            //1.key是否已经存在，并分别处理两种情况
            //查找key是否存在hash_find(key,current_offset,previous_offset)
            
            int ret = hash_find(key,current_offset,previous_offset);
            //说明找到了
            if(ret == TFS_SUCCESS)
            {
                return EXIT_META_UNEXPECT_FOUND_ERROR;
            }
            //如果不是没找到那就就是报错
            else if(ret!=EXIT_META_NOT_FOUND_ERROR)
            {
                return ret;
            }
            //如果不存在就将metaInfo写入到哈希表中hash_insert(meta,slot,previous_offset)
            ret = hash_insert(key,previous_offset,meta);
            return ret;
        }

        //将对应key的所属meta进行取出
        int32_t IndexHandle::read_segment_meta(const uint64_t key,MetaInfo& meta)
        {
            int32_t current_offset = 0;
            int32_t previous_offset = 0;
            
            int32_t ret =hash_find(key,current_offset,previous_offset);
            //说明存在key
            if(ret == TFS_SUCCESS)
            {
                ret = file_op_->pread_file(reinterpret_cast<char*>(&meta),sizeof(MetaInfo),current_offset);
                return ret;
            }
            return ret;
        }

        int32_t IndexHandle::get_free_meta_offset()const
        {
            return reinterpret_cast<IndexHeader*>(file_op_->get_map_data())->free_head_offset_;
        }

        //删除metaInfo
        int32_t IndexHandle::delete_segment_meta(const uint64_t key)
        {
            int32_t current_offset = 0;
            int32_t previous_offset = 0;
            int32_t ret =hash_find(key,current_offset,previous_offset);

            if(ret!=TFS_SUCCESS)
            {
                return ret;
            }
            MetaInfo meta;
            MetaInfo previous_meta;
            ret = file_op_->pread_file(reinterpret_cast<char*>(&meta),sizeof(MetaInfo),current_offset);
            if(ret!=TFS_SUCCESS)
            {
                return ret;
            }

            int32_t next_pos = meta.get_next_meta_offset();
            if(previous_offset == 0)
            {
                int32_t slot= static_cast<uint32_t>(key)%bucket_size();
                bucket_slot()[slot] = next_pos;
            }else{
                ret = file_op_->pread_file(reinterpret_cast<char*>(&previous_meta),sizeof(MetaInfo),previous_offset);
                if(ret!=TFS_SUCCESS)
                {
                    return ret;
                }  
                //将previous_meta节点重新写回去
                previous_meta.set_next_meta_offset(next_pos);
                ret = file_op_->pwrite_file(reinterpret_cast<char*>(&previous_meta),sizeof(MetaInfo),previous_offset);
                if(ret!=TFS_SUCCESS)
                {
                    return ret;
                }
                
            }
            //可重用链表的插入
            meta.set_next_meta_offset(this->get_free_meta_offset());
            ret = file_op_->pwrite_file(reinterpret_cast<char*>(&meta),sizeof(MetaInfo),current_offset);
            if(ret!=TFS_SUCCESS)
            {
                return ret;
            }
            index_header()->free_head_offset_=current_offset;
            if(debug)
            {
                fprintf(stdout,"delete_segment_meta delete metainfo,current_offset:%d\n",current_offset);
            }
            update_block_info(largefile::C_OPER_DELETE,meta.get_size());
            return TFS_SUCCESS;
        }
        
        //hash查找函数
        int IndexHandle::hash_find(const uint64_t key,int32_t& current_offset,int32_t& previous_offset)
        {
            int ret = TFS_SUCCESS;
            MetaInfo meta_info;
            current_offset = 0;
            previous_offset = 0;

            //1.确定key存放的桶slot的位置
            int32_t slot = static_cast<int32_t>(key)%bucket_size();
            //2.读取桶首节点存储的第一个节点的偏移量（如果偏移量为0，即该节点还没开始存文件）
            int32_t pos = bucket_slot()[slot];
            //3.根据偏移量读取存储的metaInfo
            //4.与key进行比较，相等就设置相应的offset以及previous_offset并返回TFS_SUCCESS，否则执行第5步
            //5.从metaInfo中取得下一个节点的偏移量，如果偏移值为null,那就是没找到对应meta，否则跳转至3继续执行
            while(pos!=0)
            {
                ret = file_op_->pread_file(reinterpret_cast<char*>(&meta_info),sizeof(MetaInfo),pos);
                if(ret!=TFS_SUCCESS)
                {
                    return ret;
                }

                //如果为true
                if(hash_compare(key,meta_info.get_key()))
                {
                    current_offset = pos;
                    return TFS_SUCCESS;
                }
                previous_offset = pos;
                pos = meta_info.get_next_meta_offset();
            }
            
            //那么就是没找到，说明该key还不存在
            return EXIT_META_NOT_FOUND_ERROR;
            
        }
        //hash插入
        int32_t IndexHandle::hash_insert(const uint32_t key,int32_t& previous_offset,MetaInfo&meta)
        {
            int ret = TFS_SUCCESS;
            int32_t current_offset;
            MetaInfo tmp_meta_info;
            //1.确定key存放hash桶的位置
            int32_t slot = static_cast<uint32_t>(key)%bucket_size();
            //2.确定metaInfo存储在文件中的偏移量
            //2.1考虑有可重用节点的情况
            if(this->get_free_meta_offset()!=0)
            {
                ret = file_op_->pread_file(reinterpret_cast<char*>(&tmp_meta_info),sizeof(MetaInfo),this->get_free_meta_offset());
                if(ret!=TFS_SUCCESS)
                {
                    return ret;
                }
                current_offset = index_header()->free_head_offset_;
                //调整可重用节点的偏移
                index_header()->free_head_offset_ = tmp_meta_info.get_next_meta_offset();  
                if(debug)
                {
                    fprintf(stdout,"reuse metainfo,current_offset:%d\n",current_offset);
                }
            }
            else{
                current_offset = index_header()->index_file_size_;
                index_header()->index_file_size_+=sizeof(MetaInfo);
            }
            
            //3.将metaInfo写入索引文件中
            //将下一个节点的偏移量置零
            meta.set_next_meta_offset(0);
            ret = file_op_->pwrite_file(reinterpret_cast<const char*>(&meta),sizeof(MetaInfo),current_offset);
            if(ret!=TFS_SUCCESS)
            {
                index_header()->index_file_size_-=sizeof(MetaInfo);
                return ret;
            }

            //4. 将meta节点插入到哈希链表中
            //当前个节点已经存在
            if(previous_offset!=0)
            {
                ret = file_op_->pread_file(reinterpret_cast<char*>(&tmp_meta_info),sizeof(MetaInfo),previous_offset);
                if(ret!=TFS_SUCCESS)
                {
                    index_header()->index_file_size_-=sizeof(MetaInfo);
                    return ret;
                }
                tmp_meta_info.set_next_meta_offset(current_offset);
                //更新previous节点，将数据写回去
                ret = file_op_->pwrite_file(reinterpret_cast<char*>(&tmp_meta_info),sizeof(MetaInfo),previous_offset);
                if(ret!=TFS_SUCCESS)
                {
                    index_header()->index_file_size_-=sizeof(MetaInfo);
                    return ret;
                }
            }
            //如果是该hash桶的首个节点
            else{
                bucket_slot()[slot] = current_offset;
            }
            return TFS_SUCCESS;
        }
    }
}