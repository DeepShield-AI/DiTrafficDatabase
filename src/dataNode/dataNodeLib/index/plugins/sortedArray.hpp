#pragma once
#include "../invertedIndexBuilder.hpp"
#include "../invertedIndexLoader.hpp"

#pragma pack(push, 1)
template<typename T>
struct SortedArrayEntry{
    T key;
    RowId value;
};
#pragma pack(pop)

template<typename T>
class SortedArrayIndexBlock: public TypedInvertedIndexBlock<T>{
    SortedArrayEntry<T>* entries;
    u_int64_t entryNum;

    void sort(){
        std::sort(
            this->entries, 
            this->entries + this->entryNum,
            [](const SortedArrayEntry<T>& a,const SortedArrayEntry<T>& b) {
                return a.key < b.key;
            }
        );
    }
public:
    SortedArrayIndexBlock(u_int64_t buffer_size): TypedInvertedIndexBlock<T>(buffer_size,InvertedKind::SORTED_ARRAY){
        this->entries = reinterpret_cast<SortedArrayEntry<T>*>(this->buffer + sizeof(InvertedIndexBlockInfo));
        if((buffer_size - sizeof(InvertedIndexBlock)) % sizeof(SortedArrayEntry(T)) != 0){
            throw std::invalid_argument("Error buffer size");
        }
        this->entryNum = (buffer_size - sizeof(InvertedIndexBlock)) / sizeof(SortedArrayEntry(T));
    }
    explicit SortedArrayIndexBlock(char* buffer): TypedInvertedIndexBlock<T>(buffer){
        this->entries = reinterpret_cast<SortedArrayEntry<T>*>(this->buffer + sizeof(InvertedIndexBlockInfo));
        auto buffer_size = this->bytes();
        if((buffer_size - sizeof(InvertedIndexBlock)) % sizeof(SortedArrayEntry(T)) != 0){
            throw std::invalid_argument("Error buffer size");
        }
        this->entryNum = (buffer_size - sizeof(InvertedIndexBlock)) / sizeof(SortedArrayEntry(T));
    }
    void build(std::shared_ptr<Memtable> memtable,const std::string& column_name) override{
        auto table = memtable->getTable();
        auto column_id = memtable->getColumnId(column_name);
        u_int64_t row_id = 0;
        for (auto row:table){
            T key = row.data[column_id];
            this->entries[row_id].key = key;
            this->entries[row_id].value = row_id;
            row_id ++;
        }
        this->sort();
    }
    void lookup(const T& key, std::vector<RowId>& result) const override{
        auto begin = this->entries;
        auto end   = this->entries + this->entryNum;
        auto it = std::lower_bound(begin,end,key,
            [](const SortedArrayEntry<T>& entry,const T& value) {
                return entry.key < value;
            }
        );

        while (it != end && it->key == key) {
            result.push_back(it->row_id);
            ++it;
        }
    }
};

template<typename T>
class SortedArrayIndexBuilder: public InvertedIndexBuilder{
public:
    std::unique_ptr<InvertedIndexBlock> build(std::shared_ptr<Memtable> memtable,const std::string& column_name) override{
        // 1️⃣ 从 memtable 提取列数据
        auto table = memtable->getTable();
        auto row_count = table.size();
        u_int64_t array_size = row_count * sizeof(SortedArrayEntry<T>);
        
        SortedArrayIndexBlock* block = new SortedArrayIndexBlock(array_size + sizeof(InvertedIndexBlockInfo));

        block->build(memtable, column_name);

        return std::make_unique<InvertedIndexBlock>(block);
    }
};
