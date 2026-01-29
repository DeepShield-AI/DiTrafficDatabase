#pragma once
#include "dataNodeLib/dataNodeComponent.hpp"
#include "dataNodeLib/dataNodeSignal.hpp"
#include "../lib/pipeRing.hpp"

class MitoEngine: public DataNodeComponent{
private:
    PipeRing* serverPipe;
    std::vector<PipeRing*> workerPipes;
    // std::unordered_map<u_int64_t, u_int64_t> regionWorkerMap; // regionID -> worker index
    u_int64_t regionIDToWorkerID(u_int64_t regionID, u_int64_t workerCount);
    MitoSignal* getSignal();
    void handleSignal(MitoSignal* signal);
public:
    MitoEngine(const std::string& name, const std::string& logPath, const u_int64_t id);
    ~MitoEngine() = default;
    void init(DataNodeContext& cfg) override;
    void run() override;
};
