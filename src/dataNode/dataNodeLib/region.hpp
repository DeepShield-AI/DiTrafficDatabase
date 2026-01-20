#pragma once
#include "memtable.hpp"
#include "SST.hpp"

class Region{
private:
    u_int64_t regionID;
    std::string regionName;
    std::string partition_expr;
    Memtable* current_memtable;
    SST* current_sst;
public:
    Region(/* args */);
    ~Region();
};
