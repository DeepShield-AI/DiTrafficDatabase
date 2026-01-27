#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <cstdlib>
#include <cstring>
#include "memtable.hpp"

#pragma pack(push, 1)
struct BlockFixedMeta{
    u_int64_t block_size;
    u_int64_t fixed_meta_size;
    u_int64_t var_meta_size;
    u_int64_t fixed_data_size;
    u_int64_t var_data_size;
    u_int64_t row_count;
    u_int64_t col_count;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct VarColumnInfo{
    u_int64_t offset;
    u_int64_t size;
};
#pragma pack(pop)

struct BlockVarMeta{
    enum ColumnType* column_types;
    u_int64_t* column_size;
    u_int64_t* column_name_offsets; //column name size + 1
    char* column_names;
    BlockVarMeta(){
        this->column_types = nullptr;
        this->column_size = nullptr;
        this->column_name_offsets = nullptr;
        this->column_names = nullptr;
    }
    BlockVarMeta(char* buffer,u_int64_t col_count){
        this->column_types = (enum ColumnType*)buffer;
        this->column_size = (u_int64_t*)(buffer + sizeof(enum ColumnType) * col_count);
        this->column_name_offsets = (u_int64_t*)(buffer + sizeof(enum ColumnType) * col_count + sizeof(u_int64_t) * col_count);
        this->column_names = buffer + sizeof(enum ColumnType) * col_count + sizeof(u_int64_t) * col_count + sizeof(u_int64_t) * (col_count + 1);
    }
    static u_int64_t BasicSize(u_int64_t col_count){
        return sizeof(enum ColumnType) * col_count + sizeof(u_int64_t) * col_count + sizeof(u_int64_t) * (col_count + 1);
    }
    void serialize(std::unordered_map<std::string, ColumnType>& columnNames){
        if (this->column_types == nullptr || this->column_size == nullptr || this->column_name_offsets == nullptr || this->column_names == nullptr){
            throw std::runtime_error("BlockVarMeta not initialized");
        }
        u_int64_t idx = 0;
        u_int64_t name_offset = 0;
        for(auto [col_name, col_type] : columnNames){
            this->column_types[idx] = col_type;
            this->column_name_offsets[idx] = name_offset;
            std::memcpy(this->column_names + name_offset, col_name.c_str(), col_name.size());
            if(col_type == ColumnType::STRING){
                this->column_size[idx] = sizeof(VarColumnInfo);
            }else if(col_type == ColumnType::INT64){
                this->column_size[idx] += sizeof(int64_t);
            }else if(col_type == ColumnType::UINT64){
                this->column_size[idx] += sizeof(u_int64_t);
            }else if(col_type == ColumnType::DOUBLE){
                this->column_size[idx] += sizeof(double);
            }else{
                throw std::invalid_argument("Unknown column type");
            }
            name_offset += col_name.size();
            idx++;
        }
        this->column_name_offsets[idx] = name_offset; // end offset
    }
};

struct FixedBlock{
    char* rows;
    void write(u_int64_t rowID, u_int64_t rowSize, u_int64_t colOffset, u_int64_t colSize, char* data){
        u_int64_t offset = rowSize * rowID + colOffset;
        std::memcpy(this->rows + offset, data, colSize);
    }
    char* read(u_int64_t rowID, u_int64_t rowSize, u_int64_t colOffset){
        u_int64_t offset = rowSize * rowID + colOffset;
        return this->rows + offset;
    }
};

struct VarBlock{
    char* buffer;
    void write(char* data, u_int64_t size, u_int64_t offset){
        std::memcpy(this->buffer + offset, data, size);
    }
    char* read(u_int64_t offset){
        return this->buffer + offset;
    }
};

class SSTBlock{
private:
    char* buffer;
    BlockFixedMeta* fixed_meta;
    BlockVarMeta var_meta;
    FixedBlock fixed_block;
    VarBlock var_block;
public:
    SSTBlock(u_int64_t buffer_size){
        this->buffer = new char[buffer_size];
        this->fixed_meta = (BlockFixedMeta*)this->buffer;
        this->fixed_meta->block_size = buffer_size;
        this->fixed_meta->fixed_meta_size = sizeof(BlockFixedMeta);
        this->var_meta = BlockVarMeta();
        this->fixed_block = FixedBlock();
        this->var_block = VarBlock();
    }
    SSTBlock(char* buffer, u_int64_t buffer_size){
        this->buffer = buffer;
        this->fixed_meta = (BlockFixedMeta*)this->buffer;
        this->var_meta = BlockVarMeta(this->buffer + sizeof(BlockFixedMeta), this->fixed_meta->col_count);
        this->fixed_block = FixedBlock();
        this->fixed_block.rows = this->buffer + sizeof(BlockFixedMeta) + this->fixed_meta->var_meta_size;
        this->var_block = VarBlock();
        this->var_block.buffer = this->buffer + sizeof(BlockFixedMeta) + this->fixed_meta->var_meta_size + this->fixed_meta->fixed_data_size;
    }
    ~SSTBlock(){
        delete[] this->buffer;
    }
    void serializeMemtable(std::shared_ptr<Memtable> memtable){
        auto column_names = memtable->getColumnNames();
        auto table = memtable->getTable();
        this->fixed_meta->row_count = table.size();
        this->fixed_meta->col_count = column_names.size();
        this->fixed_meta->var_meta_size = BlockVarMeta::BasicSize(this->fixed_meta->col_count) + memtable->columnNameSize();

        // Calculate fixed data size
        u_int64_t fixed_row_size = 0;
        std::vector<u_int64_t> col_offsets;
        for(auto [col_name, col_type] : column_names){
            col_offsets.push_back(fixed_row_size);
            if(col_type == ColumnType::STRING){
                fixed_row_size += sizeof(VarColumnInfo);
            }else if(col_type == ColumnType::INT64){
                fixed_row_size += sizeof(int64_t);
            }else if(col_type == ColumnType::UINT64){
                fixed_row_size += sizeof(u_int64_t);
            }else if(col_type == ColumnType::DOUBLE){
                fixed_row_size += sizeof(double);
            }else{
                throw std::invalid_argument("Unknown column type");
            }
        }
        this->fixed_meta->fixed_data_size = fixed_row_size * this->fixed_meta->row_count;

        this->fixed_meta->var_data_size = memtable->varCapacity();

        // Fill VarMeta
        this->var_meta = BlockVarMeta(this->buffer + sizeof(BlockFixedMeta), this->fixed_meta->col_count);
        try{
            this->var_meta.serialize(column_names);
        } catch (const std::runtime_error& e){
            throw e;
        }

        // Fill FixedBlock and VarBlock
        u_int64_t rowID = 0;
        u_int64_t var_data_offset = 0;
        for(auto row:table){
            for(u_int64_t idx = 0; idx < this->fixed_meta->col_count; idx++){
                ColumnType col_type = this->var_meta.column_types[idx];
                u_int64_t col_size = this->var_meta.column_size[idx];
                Value val = row.data[idx];
                if(col_type == ColumnType::STRING){
                    std::string str_val = std::get<std::string>(val);
                    VarColumnInfo var_info;
                    var_info.offset = this->fixed_meta->var_data_size;
                    var_info.size = str_val.size();
                    // Write VarColumnInfo to FixedBlock
                    this->fixed_block.write(rowID, fixed_row_size, col_offsets[idx], col_size, (char*)&var_info);
                    // Write actual string data to VarBlock
                    this->var_block.write((char*)str_val.c_str(), str_val.size(), var_data_offset);
                    var_data_offset += str_val.size();
                }else if(col_type == ColumnType::INT64){
                    int64_t int_val = std::get<int64_t>(val);
                    this->fixed_block.write(rowID, fixed_row_size, col_offsets[idx], col_size, (char*)&int_val);
                }else if(col_type == ColumnType::UINT64){
                    u_int64_t uint_val = std::get<u_int64_t>(val);
                    this->fixed_block.write(rowID, fixed_row_size, col_offsets[idx], col_size, (char*)&uint_val);
                }else if(col_type == ColumnType::DOUBLE){
                    double double_val = std::get<double>(val);
                    this->fixed_block.write(rowID, fixed_row_size, col_offsets[idx], col_size, (char*)&double_val);
                }else{
                    throw std::invalid_argument("Unknown column type");
                }
            }
            rowID++;
        }
    }
    char* data() const{
        return this->buffer;
    }
};


class SSTBlockMeta{
private:
    std::string fileFolder;
    std::string fileName;
    u_int64_t fileOffset;
    u_int64_t start_time;
    u_int64_t end_time;
    SSTBlock* block;
    bool stored;
public:
    SSTBlockMeta(std::string fileFolder, std::string fileName, u_int64_t fileOffset, u_int64_t start_time, u_int64_t end_time){
        this->fileFolder = fileFolder;
        this->fileName = fileName;
        this->fileOffset = fileOffset;
        this->start_time = start_time;
        this->end_time = end_time;
        this->block = nullptr;
        this->stored = false;
    }
    ~SSTBlockMeta(){
        if(this->block != nullptr){
            delete this->block;
        }
    }
    std::shared_ptr<SSTBlock> loadBlock(){
        std::string fullPath = this->fileFolder + "/" + this->fileName;
        std::ifstream infile(fullPath, std::ios::binary);
        if(!infile.is_open()){
            throw std::runtime_error("Failed to open SST file: " + fullPath);
        }
        infile.seekg(0, std::ios::end);
        u_int64_t file_size = infile.tellg();
        if(this->fileOffset >= file_size){
            infile.close();
            throw std::out_of_range("File offset is out of range");
        }
        infile.seekg(this->fileOffset, std::ios::beg);
        // Read BlockFixedMeta first
        BlockFixedMeta fixed_meta;
        infile.read((char*)&fixed_meta, sizeof(BlockFixedMeta));
        // Read the entire block
        infile.seekg(this->fileOffset, std::ios::beg);
        char* buffer = new char[fixed_meta.block_size];
        infile.read(buffer, fixed_meta.block_size);
        infile.close();
        this->block = new SSTBlock(buffer, fixed_meta.block_size);
        return std::shared_ptr<SSTBlock>(this->block);
    }
    void flushBlock(std::shared_ptr<Memtable> memtable){
        if(this->stored){
            throw std::runtime_error("Block already stored to disk");
        }
        u_int64_t disk_size = 0;
        auto column_names = memtable->getColumnNames();
        auto table = memtable->getTable();
        auto row_count = table.size();
        auto col_count = column_names.size();
        disk_size += sizeof(BlockFixedMeta);
        disk_size += BlockVarMeta::BasicSize(col_count) + memtable->columnNameSize();
        // Calculate fixed data size
        u_int64_t fixed_row_size = 0;
        std::vector<u_int64_t> col_offsets;
        for(auto [col_name, col_type] : column_names){
            col_offsets.push_back(fixed_row_size);
            if(col_type == ColumnType::STRING){
                fixed_row_size += sizeof(VarColumnInfo);
            }else if(col_type == ColumnType::INT64){
                fixed_row_size += sizeof(int64_t);
            }else if(col_type == ColumnType::UINT64){
                fixed_row_size += sizeof(u_int64_t);
            }else if(col_type == ColumnType::DOUBLE){
                fixed_row_size += sizeof(double);
            }else{
                throw std::invalid_argument("Unknown column type");
            }
        }
        disk_size += fixed_row_size * row_count;
        disk_size += memtable->varCapacity();
        
        SSTBlock* new_block = new SSTBlock(disk_size);
        try{
            new_block->serializeMemtable(memtable);
        } catch (const std::runtime_error& e){
            delete new_block;
            throw e;
        }
        // Write to file
        std::string fullPath = this->fileFolder + "/" + this->fileName;
        std::ofstream outfile(fullPath, std::ios::binary | std::ios::app);
        if(!outfile.is_open()){
            delete new_block;
            throw std::runtime_error("Failed to open SST file for writing: " + fullPath);
        }
        outfile.seekp(0, std::ios::end);
        this->fileOffset = outfile.tellp();
        outfile.write(new_block->data(), disk_size);
        outfile.close();
        this->block = new_block;
        this->stored = true;
    }
    void deleteCache(){
        if(this->block != nullptr){
            delete this->block;
            this->block = nullptr;
        }
    }
    std::string Log() const{
        if(!this->stored){
            return std::string();
        }
        return "SSTBlockMeta(file: " + this->fileName + ", offset: " + std::to_string(this->fileOffset) + ", time_range: [" + std::to_string(this->start_time) + ", " + std::to_string(this->end_time) + "])";
    }
};

class SST{
private:
    std::vector<SSTBlockMeta> region_metas;
    std::string logPath;
    std::ofstream logFile;
public:
    SST(std::string logPath):logPath(logPath){
        this->region_metas = std::vector<SSTBlockMeta>();
        this->logFile.open(this->logPath + "/sst_log.txt", std::ios::app);
        if(!this->logFile.is_open()){
            throw std::runtime_error("Failed to open SST log file");
        }
    }
    ~SST(){
        if(this->logFile.is_open()){
            this->logFile.close();
        }
    }
    void appendMemtable(std::shared_ptr<Memtable> memtable, u_int64_t start_time, u_int64_t end_time, u_int64_t regionID){
        std::string fileName = "sst_" + std::to_string(regionID) + ".sst";
        SSTBlockMeta meta(this->logPath, fileName, 0, start_time, end_time);
        try{
            meta.flushBlock(memtable);
        } catch (const std::runtime_error& e){
            throw e;
        }
        this->logFile << meta.Log() << std::endl;
        this->region_metas.push_back(meta);
        meta.deleteCache();
    }
};