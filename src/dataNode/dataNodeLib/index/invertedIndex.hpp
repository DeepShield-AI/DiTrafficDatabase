#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <fstream>
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
    const char* getBuffer() const {return this->buffer;}
    u_int64_t bytes() const { return meta->size; }
    ValueType value_type() const { return static_cast<ValueType>(meta->valueType); }
    InvertedKind inverted_kind() const { return static_cast<InvertedKind>(meta->invertedKind); }
    virtual void lookup_raw(const void* key, std::vector<RowId>& result) const = 0;
    // virtual void build(std::shared_ptr<Memtable> memtable,const std::string& column_name) = 0;
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
    TypedInvertedIndexBlock(u_int64_t buffer_size, InvertedKind kind): InvertedIndexBlock(buffer_size){
        meta->valueType = static_cast<u_int64_t>(ValueTypeTrait<T>::type);
        meta->invertedKind = static_cast<u_int64_t>(kind);
    }

    explicit TypedInvertedIndexBlock(char* buffer): InvertedIndexBlock(buffer) {}

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
    void flushBlock(std::shared_ptr<Memtable> memtable){
        if(this->stored){
            throw std::runtime_error("Block already stored to disk");
        }
        std::string fullPath = this->file_folder + "/" + this->file_name;

        std::ofstream out(fullPath, std::ios::binary | std::ios::app);

        if (!out.is_open()) {
            throw std::runtime_error("Cannot open file");
        }

        u_int64_t current_offset = this->file_offset;

        auto column_names = memtable->getColumnNames();
        auto table = memtable->getTable();
        auto row_count = table.size();
        auto col_count = column_names.size();

        for(auto [col_name, col_type] : column_names) {
            InvertedKind kind = InvertedKind::SORTED_ARRAY;
            if(col_type == ValueType::UINT64){
                InvertedKind kind = InvertedKind::SORTED_ARRAY;
            } else if(col_type == ValueType::INT64){
                InvertedKind kind = InvertedKind::SORTED_ARRAY;
            } else if(col_type == ValueType::DOUBLE){
                InvertedKind kind = InvertedKind::SORTED_ARRAY;
            } else if(col_type == ValueType::STRING){
                InvertedKind kind = InvertedKind::TRIE;
            } else {
                throw std::invalid_argument("Unknown column type");
            }

            auto builder = InvertedIndexBuilderRegistry::instance().create(col_type, kind);
            
            auto block = builder->build(memtable, col_name);
            out.write(block->getBuffer(),block->bytes());
            this->column_index_offset[col_name] =current_offset;
            current_offset += block->bytes();
        }

        out.close();
        this->stored = true;
    }
    std::shared_ptr<InvertedIndexBlock> IndexBlockMeta::loadBlock(std::string columnName){
        // 缓存命中
        auto it_cache = column_index_cache.find(columnName);
        if (it_cache == column_index_cache.end()){
            throw std::invalid_argument("Unknown column name");
        }
        if (it_cache->second != nullptr) {
            return it_cache->second;
        }

        auto it = column_index_offset.find(columnName);
        if (it == column_index_offset.end()) {
            throw std::runtime_error("Column index not found");
        }

        u_int64_t offset = it->second;

        std::string file_path = this->file_folder + "/" + this->file_name;

        std::ifstream in(file_path,std::ios::binary);

        if (!in.is_open()) {
            throw std::runtime_error("Cannot open file");
        }

        in.seekg(offset);

        InvertedIndexBlockInfo meta;
        in.read(reinterpret_cast<char*>(&meta),sizeof(meta));

        // 2️⃣ 读取完整 block
        char* buffer = new char[meta.size];

        in.seekg(offset);
        in.read(buffer, meta.size);

        in.close();

        // 3️⃣ 调用 loader
        auto block = InvertedIndexLoaderRegistry::instance().create(static_cast<ValueType>(meta.valueType),static_cast<InvertedKind>(meta.invertedKind),buffer);

        // 4️⃣ 缓存
        column_index_cache[columnName] =std::shared_ptr<InvertedIndexBlock>(block.release());

        return std::shared_ptr<InvertedIndexBlock>(column_index_cache[columnName].get());
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

class Index{
private:
    u_int64_t regionID;
    std::vector<IndexBlockMeta> region_metas;
    std::string dataPath;
    std::string logPath;
    std::ofstream logFile;
public:
    Index(u_int64_t regionID, std::string dataPath, std::string logPath):regionID(regionID),dataPath(dataPath),logPath(logPath){
        // this->region_sst_metas = std::unordered_map<u_int64_t, std::vector<SSTBlockMeta>>();
        this->region_metas = std::vector<IndexBlockMeta>();
        this->logFile.open(this->logPath + "/index.log", std::ios::app);
        if(!this->logFile.is_open()){
            throw std::runtime_error("Failed to open index log file");
        }
        this->log("Create index for region " + std::to_string(regionID));
    }
    ~Index(){
        this->region_metas.clear();
        if(this->logFile.is_open()){
            this->logFile.close();
        }
    }
    void appendMemtable(std::shared_ptr<Memtable> memtable, u_int64_t start_time, u_int64_t end_time, u_int64_t regionID){
        std::string fileName = "index_" + std::to_string(regionID) + ".idx";
        IndexBlockMeta meta(dataPath, fileName, 0, start_time, end_time);
        try{
            meta.flushBlock(memtable);
        } catch (const std::runtime_error& e){
            throw e;
        }
        this->log(meta.Log());
        // meta.deleteCache();
        this->region_metas.push_back(meta);
    }
    void log(const std::string& message){
        std::string log_message = "[Index] " + message;
        if(this->logFile.is_open()){
            this->logFile << log_message << std::endl;
        }
    }
};
