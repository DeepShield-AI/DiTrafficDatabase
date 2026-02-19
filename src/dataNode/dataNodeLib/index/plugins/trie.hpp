#pragma once
#include "../invertedIndexBuilder.hpp"
#include "../invertedIndexLoader.hpp"

template<typename T>
class TrieIndexBlock: public TypedInvertedIndexBlock<T>{
public:
    // 构建阶段使用
    TrieIndexBlock(u_int64_t buffer_size): TypedInvertedIndexBlock<T>(buffer_size,InvertedKind::SORTED_ARRAY){
        // 初始化内部结构
    }

    // 磁盘恢复阶段使用
    explicit TrieIndexBlock(char* buffer): TypedInvertedIndexBlock<T>(buffer){
        // 从 buffer 解析内部结构
    }

    void lookup(const T& key, std::vector<RowId>& result) const override{
        // 在内部结构中查找 key
        // 将 RowId 填入 result
    }
};

template<typename T>
class TrieIndexBuilder: public InvertedIndexBuilder{
public:
    std::unique_ptr<InvertedIndexBlock>
    build(std::shared_ptr<Memtable> memtable,const std::string& column_name) override{
        // 1️⃣ 从 memtable 提取列数据

        // 2️⃣ 构建索引内部结构
        //    （排序、分组等）

        // 3️⃣ 计算所需 buffer 大小

        // 4️⃣ 创建 Block

        // 5️⃣ 将数据写入 block->buffer

        return block;
    }
};
