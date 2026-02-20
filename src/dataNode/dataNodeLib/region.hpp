#pragma once
#include "memtable.hpp"
#include "SST.hpp"
#include "index/invertedIndex.hpp"

class Region{
private:
    u_int64_t regionID;
    std::string regionName;
    std::string partition_expr;
    std::shared_ptr<SST> current_sst; // stay in memory util drop region
    std::shared_ptr<Index> current_index; // stay in memory util drop region
    std::unordered_map<std::string, std::string> attrs; // memtable threshold, sst path, column names, etc.
    std::shared_ptr<Memtable> current_memtable;
public:
    Region(u_int64_t regionID, std::string regionName, std::string partition_expr, std::unordered_map<std::string, std::string>& attrs){
        this->regionID = regionID;
        this->regionName = regionName;
        this->partition_expr = partition_expr;
        this->current_memtable = nullptr;
        this->current_sst = nullptr;
        this->current_index = nullptr;
        this->attrs = attrs;
    }
    ~Region()=default;
    u_int64_t getRegionID() const{
        return this->regionID;
    }
    std::string getRegionName() const{
        return this->regionName;
    }
    std::string getPartitionExpr() const{
        return this->partition_expr;
    }
    void setMemtable(std::shared_ptr<Memtable> memtable){
        this->current_memtable = memtable;
    }
    std::shared_ptr<Memtable> getMemtable() const{
        return this->current_memtable;
    }
    std::string getAtrr(const std::string& key) const{
        auto it = this->attrs.find(key);
        if(it != this->attrs.end()){
            return it->second;
        }
        return "";
    }
    void setSST(std::shared_ptr<SST> sst){
        this->current_sst = sst;
    }
    std::shared_ptr<SST> getSST() const{
        return this->current_sst;
    }
    void setIndex(std::shared_ptr<Index> index){
        this->current_index = index;
    }
    std::shared_ptr<Index> getIndex() const{
        return this->current_index;
    }
};
