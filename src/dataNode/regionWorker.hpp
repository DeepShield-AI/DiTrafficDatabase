#pragma once
#include "dataNodeLib/dataNodeComponent.hpp"
#include "dataNodeLib/dataNodeSignal.hpp"
#include "dataNodeLib/request.hpp"
#include "../lib/pipeRing.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class RegionWorker: public DataNodeComponent{
private:
    PipeRing* enginePipe;
    PipeRing* flusherPipe;
    std::unordered_map<u_int64_t, std::unique_ptr<Region>> regions;
    std::unordered_map<u_int64_t, std::unique_ptr<Region>> closedRegions;
    std::unordered_map<u_int64_t, std::unique_ptr<Region>> catchupRegions;
    std::unordered_map<u_int64_t, std::unique_ptr<Region>> dropingRegions;

    WorkSiganal* getSignal();
    void handleWrite(WriteRequest& request, u_int64_t regionID, u_int64_t requestID);
    void handleStatus(RegionAdminRequest& request, u_int64_t regionID, u_int64_t requestID);
    u_int64_t checkRegionMemUsage(u_int64_t regionID);
    void sendFlushSignal(u_int64_t regionID);
    void handleSignal(WorkSiganal* signal);
public:
    RegionWorker(const std::string& name, const std::string& logPath);
    ~RegionWorker() = default;
    void init(DataNodeContext& cfg) override;
    void run() override;
};
