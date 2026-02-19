#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include "../memtable.hpp"

using RowId = u_int64_t;
using ValueType = ColumnType;

enum class InvertedKind {
    SORTED_ARRAY,
    TRIE,
};

#pragma pack(push, 1)
struct InvertedIndexBlockInfo {
    u_int64_t size;          // 整个block大小
    u_int64_t valueType;     // ValueType
    u_int64_t invertedKind;  // InvertedKind
};
#pragma pack(pop)

class InvertedIndexBlock {
protected:
    char* buffer;
    InvertedIndexBlockInfo* meta;
public:
    explicit InvertedIndexBlock(u_int64_t buffer_size) {
        buffer = new char[buffer_size];
        meta = reinterpret_cast<InvertedIndexBlockInfo*>(buffer);
        meta->size = buffer_size;
    }
    explicit InvertedIndexBlock(char* buffer)
        : buffer(buffer),
          meta(reinterpret_cast<InvertedIndexBlockInfo*>(buffer)) {}
    virtual ~InvertedIndexBlock() = default;
    u_int64_t bytes() const { return meta->size; }
    ValueType value_type() const { return static_cast<ValueType>(meta->valueType); }
    InvertedKind inverted_kind() const { return static_cast<InvertedKind>(meta->invertedKind); }
    virtual void lookup_raw(const void* key, std::vector<RowId>& result) const = 0;
};

template<typename T>
struct ValueTypeTrait;

template<> struct ValueTypeTrait<u_int64_t> { static constexpr ValueType type = ValueType::UINT64; };
template<> struct ValueTypeTrait<int64_t>  { static constexpr ValueType type = ValueType::INT64; };
template<> struct ValueTypeTrait<double>   { static constexpr ValueType type = ValueType::DOUBLE; };
template<> struct ValueTypeTrait<std::string> { static constexpr ValueType type = ValueType::STRING; };

template<typename T>
class TypedInvertedIndexBlock : public InvertedIndexBlock {
protected:
    TypedInvertedIndexBlock(u_int64_t buffer_size, InvertedKind kind)
        : InvertedIndexBlock(buffer_size)
    {
        meta->valueType = static_cast<u_int64_t>(ValueTypeTrait<T>::type);
        meta->invertedKind = static_cast<u_int64_t>(kind);
    }

    explicit TypedInvertedIndexBlock(char* buffer)
        : InvertedIndexBlock(buffer) {}

    const T& key_cast(const void* key) const {
        return *reinterpret_cast<const T*>(key);
    }

public:
    virtual void lookup(const T& key, std::vector<RowId>& result) const = 0;

    void lookup_raw(const void* key, std::vector<RowId>& result) const override {
        lookup(key_cast(key), result);
    }
};

class IndexBlockMeta {
private:
    // 文件信息
    std::string file_folder;
    std::string file_name;
    u_int64_t file_offset;      // 数据块起始位置

    // 时间范围（用于时间裁剪）
    u_int64_t start_time;
    u_int64_t end_time;

    // 每个列的索引在文件中的偏移
    std::unordered_map<std::string, u_int64_t> column_index_offset;
    // 已加载的缓存
    std::unordered_map<std::string,std::shared_ptr<InvertedIndexBlock>> column_index_cache;

    bool stored;

public:
    IndexBlockMeta(const std::string& folder, const std::string& name, u_int64_t offset,u_int64_t start,u_int64_t end)
        : file_folder(folder),file_name(name),file_offset(offset),start_time(start),end_time(end),stored(false) {}
    std::unique_ptr<InvertedIndexBlock> loadBlock(std::string colunmName){

    }
    void flushBlock(std::shared_ptr<Memtable> memtable){
        if(this->stored){
            throw std::runtime_error("Block already stored to disk");
        }
        
    }
    void deleteCache(){
        for(auto it:this->column_index_cache){
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
        return "IndexBlockMeta(file: " + this->file_name + ", offset: " + std::to_string(this->file_offset) + ", time_range: [" + std::to_string(this->start_time) + ", " + std::to_string(this->end_time) + "])";
    }
    std::string serialize() const{
        if(!this->stored){
            throw std::runtime_error("Block not stored to disk");
        }
        std::string ret = this->file_name + "," + std::to_string(this->file_offset) + "," + std::to_string(this->start_time) + "," + std::to_string(this->end_time) + ",";
        for(auto it:this->column_index_offset){
            ret += it.first;
            ret += ":";
            ret += std::to_string(it.second);
            ret += ",";
        }
        return ret;
    }
};
