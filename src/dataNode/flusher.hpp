#pragma once
#include "dataNodeLib/dataNodeComponent.hpp"
#include "../lib/pipeRing.hpp"

struct FlushSignal{
    u_int64_t regionID;
    u_int64_t startTime;
    u_int64_t endTime;
    std::shared_ptr<Memtable> memtable;
    std::shared_ptr<SST> sst;
};

class Flusher: public DataNodeComponent{
private:
    PipeRing* workerPipe;
    FlushSignal* getSignal();
    void handleFlush(FlushSignal* signal);
public:
    Flusher(const std::string& name, const std::string& logPath);
    ~Flusher() = default;
    void init(DataNodeContext& cfg) override;
    void run() override;
};
