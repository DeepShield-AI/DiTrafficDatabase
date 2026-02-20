#pragma once
#include "dataNodeLib/dataNodeComponent.hpp"
#include "../lib/pipeRing.hpp"
#include "dataNodeLib/dataNodeSignal.hpp"

class Indexer: public DataNodeComponent{
private:
    PipeRing* workerPipe;
    IndexSignal* getSignal();
    void handleIndex(IndexSignal* signal);
public:
    Indexer(const std::string& name, const std::string& logPath, const u_int64_t id);
    ~Indexer() = default;
    void init(DataNodeContext& cfg) override;
    void run() override;
};
