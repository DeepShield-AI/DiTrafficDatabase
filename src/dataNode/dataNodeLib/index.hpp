#pragma once
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <fstream>
#include "memtable.hpp"

using RowId = u_int64_t;
using ValueType = ColumnType;

enum class IndexKind{
    INVERTED,
    FILTER,
};

enum class InvertedKind{
    SORTEDARRAY,
    TRIE,
};

enum class FilterKind{
    BLOOM,
};

template <typename T>
struct ValueTypeTrait;

template <>
struct ValueTypeTrait<double> {
    static constexpr ValueType type = ValueType::DOUBLE;
};

template <>
struct ValueTypeTrait<u_int64_t> {
    static constexpr ValueType type = ValueType::UINT64;
};

template <>
struct ValueTypeTrait<int64_t> {
    static constexpr ValueType type = ValueType::INT64;
};

template <>
struct ValueTypeTrait<std::string> {
    static constexpr ValueType type = ValueType::STRING;
};

#pragma pack(push, 1)
struct IndexBlockInfo{
    u_int64_t size;
    u_int64_t indexKind;
    u_int64_t valueType;   
    u_int64_t invertedOrFilterKind;
};
#pragma pack(pop)

class IndexBlock {
protected:
    char* buffer;
    IndexBlockInfo* meta;
public:
    explicit IndexBlock(u_int64_t buffer_size) {
        buffer = new char[buffer_size];
        meta = reinterpret_cast<IndexBlockInfo*>(buffer);
        meta->size = buffer_size;
    }
    explicit IndexBlock(char* buffer): buffer(buffer),meta(reinterpret_cast<IndexBlockInfo*>(buffer)) {}
    virtual ~IndexBlock() = default;

    u_int64_t index_kind() const { return meta->indexKind; }
    u_int64_t value_type() const { return meta->valueType; }
    u_int64_t bytes() const { return meta->size; }
    char* data() { return buffer; }

    virtual void lookup_raw(const void* key,std::vector<RowId>& result) const = 0;
    virtual bool may_contain_raw(const void* key) const {
        return true;
    }
};


template <typename T>
class TypedIndexBlock : public IndexBlock {
protected:
    /// 构建态
    TypedIndexBlock(u_int64_t buffer_size,IndexKind kind) : IndexBlock(buffer_size) {
        meta->indexKind = static_cast<u_int64_t>(kind);
        meta->valueType =static_cast<u_int64_t>(ValueTypeTrait<T>::type);
    }
    /// 查询态
    explicit TypedIndexBlock(char* buffer): IndexBlock(buffer) {}
    const T& key_cast(const void* key) const {
        return *reinterpret_cast<const T*>(key);
    }
};

template <typename T>
class InvertedIndexBlock : public TypedIndexBlock<T> {
protected:
    /// 构建态
    explicit InvertedIndexBlock(u_int64_t buffer_size): TypedIndexBlock<T>(buffer_size,IndexKind::INVERTED) {}
    /// 查询态
    explicit InvertedIndexBlock(char* buffer): TypedIndexBlock<T>(buffer) {}
public:
    virtual void lookup(const T& key,std::vector<RowId>& result) const = 0;
    void lookup_raw(const void* key,std::vector<RowId>& result) const override {
        lookup(this->key_cast(key), result);
    }
};


template <typename T>
class FilterIndexBlock : public TypedIndexBlock<T> {
protected:
    explicit FilterIndexBlock(u_int64_t buffer_size): TypedIndexBlock<T>(buffer_size,IndexKind::FILTER) {}
    explicit FilterIndexBlock(char* buffer): TypedIndexBlock<T>(buffer) {}
public:
    virtual bool may_contain(const T& key) const = 0;
    bool may_contain_raw(const void* key) const override {
        return may_contain(this->key_cast(key));
    }
};


template <typename T>
class SortedArrayIndexBlock : public InvertedIndexBlock<T>;

template <typename T>
class TrieIndexBlock : public InvertedIndexBlock<T>;

template <typename T>
class BloomIndexBlock : public FilterIndexBlock<T>;



class IndexBlockMeta{
private:
    std::string fileFolder;
    std::string fileName;
    u_int64_t fileOffset;
    u_int64_t start_time;
    u_int64_t end_time;
    std::unordered_map<std::string, u_int64_t> columnNameIndexOffsetMap;
    std::unordered_map<std::string,std::shared_ptr<IndexBlock>> columnNameIndexMap;
    bool stored;
    IndexBlockMeta(std::string fileFolder, std::string fileName, u_int64_t fileOffset, u_int64_t start_time, u_int64_t end_time, u_int64_t block_id){
        this->fileFolder = fileFolder;
        this->fileName = fileName;
        this->fileOffset = fileOffset;
        this->start_time = start_time;
        this->end_time = end_time;
        this->stored = false;
        this->columnNameIndexMap = std::unordered_map<std::string,std::shared_ptr<IndexBlock>>();
        this->columnNameIndexOffsetMap = std::unordered_map<std::string,u_int64_t>();
    }
    ~IndexBlockMeta() = default;
    // std::shared_ptr<IndexBlock> loadBlock(std::string colunmName){
    //     if(this->columnNameIndexOffsetMap.find(colunmName) == this->columnNameIndexOffsetMap.end()){
    //         throw std::runtime_error("Non-exist column name");
    //     }
    //     std::string fullPath = this->fileFolder + "/" + this->fileName;
    //     std::ifstream infile(fullPath, std::ios::binary);
    //     if(!infile.is_open()){
    //         throw std::runtime_error("Failed to open SST file: " + fullPath);
    //     }
    //     infile.seekg(0, std::ios::end);
    //     u_int64_t file_size = infile.tellg();
    //     file_size += this->columnNameIndexOffsetMap[colunmName];
    //     if(this->fileOffset >= file_size){
    //         infile.close();
    //         throw std::out_of_range("File offset is out of range");
    //     }
    //     infile.seekg(this->fileOffset, std::ios::beg);


    //     // Read BlockFixedMeta first
    //     IndexBlockInfo info;
    //     infile.read((char*)&info, sizeof(IndexBlockInfo));
    //     // Read the entire block
    //     infile.seekg(this->fileOffset, std::ios::beg);
    //     char* buffer = new char[info.size];
    //     infile.read(buffer, info.size);
    //     infile.close();
    //     std::shared_ptr<IndexBlock> block;

    //     auto vt = static_cast<ValueType>(info.valueType);
    //     auto ik = static_cast<IndexKind>(info.indexKind);
    //     auto fk = static_cast<InvertedOrFilterKind>(info.invertedOrFilterKind);

    //     if(ik == IndexKind::INVERTED) {
    //         switch(vt) {
    //         case ValueType::UInt64:
    //             block = std::make_shared<SortedArrayIndexBlock<u_int64_t>>(buffer, info.size);
    //             break;
    //         case ValueType::Int64:
    //             block = std::make_shared<SortedArrayIndexBlock<int64_t>>(buffer, info.size);
    //             break;
    //         case ValueType::Double:
    //             block = std::make_shared<SortedArrayIndexBlock<double>>(buffer, info.size);
    //             break;
    //         case ValueType::String:
    //             block = std::make_shared<TrieIndexBlock<std::string>>(buffer, info.size);
    //             break;
    //         default:
    //             delete[] buffer;
    //             throw std::runtime_error("Unsupported ValueType");
    //         }
    //     }
    //     else if(ik == IndexKind::FILTER) {
    //         switch(vt) {
    //         case ValueType::UInt64:
    //             block = std::make_shared<BloomIndexBlock<u_int64_t>>(buffer, info.size);
    //             break;
    //         case ValueType::Int64:
    //             block = std::make_shared<BloomIndexBlock<int64_t>>(buffer, info.size);
    //             break;
    //         case ValueType::Double:
    //             block = std::make_shared<BloomIndexBlock<double>>(buffer, info.size);
    //             break;
    //         case ValueType::String:
    //             block = std::make_shared<BloomIndexBlock<std::string>>(buffer, info.size);
    //             break;
    //         default:
    //             throw std::runtime_error("Unsupported ValueType");
    //         }
    //     }
    //     else {
    //         throw std::runtime_error("Unknown IndexKind");
    //     }
    //     this->columnNameIndexMap[colunmName] = block;
    //     return sthis->columnNameIndexMap[colunmName];
    // }
    void flushBlock(std::shared_ptr<Memtable> memtable){
        if(this->stored){
            throw std::runtime_error("Block already stored to disk");
        }
        
    }
    void deleteCache(){
        for(auto it:this->columnNameIndexMap){
            if(it.second != nullptr){
                it.second.reset();
                it.second = nullptr;
            }
        }
    }

    std::string Log() const{
        if(!this->stored){
            return std::string();
        }
        return "IndexBlockMeta(file: " + this->fileName + ", offset: " + std::to_string(this->fileOffset) + ", time_range: [" + std::to_string(this->start_time) + ", " + std::to_string(this->end_time) + "])";
    }
    std::string serialize() const{
        if(!this->stored){
            throw std::runtime_error("Block not stored to disk");
        }
        std::string ret = this->fileName + "," + std::to_string(this->fileOffset) + "," + std::to_string(this->start_time) + "," + std::to_string(this->end_time) + ",";
        for(auto it:this->columnNameIndexOffsetMap){
            ret += it.first;
            ret += ":";
            ret += std::to_string(it.second);
            ret += ",";
        }
    }
};

