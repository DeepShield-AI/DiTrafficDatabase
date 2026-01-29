#pragma once
#include "dataNodeComponent.hpp"
#include "request.hpp"

struct MitoSignal{
    u_int64_t regionID;
    Request request;
};

struct WorkSignal{
    u_int64_t regionID;
    Request request;
};

struct FlushSignal{
    u_int64_t regionID;
    u_int64_t startTime;
    u_int64_t endTime;
    std::shared_ptr<Memtable> memtable;
    std::shared_ptr<SST> sst;
};