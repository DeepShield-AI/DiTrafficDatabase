#pragma once
#include "dataNodeLib/dataNodeComponent.hpp"
#include "../lib/pipeRing.hpp"
#include "dataNodeLib/dataNodeSignal.hpp"
#include <nlohmann/json.hpp>
#include <chrono>

using namespace std::chrono;
using json = nlohmann::json;
using Dict = std::unordered_map<std::string, std::string>;
using DictList = std::vector<Dict>;

class RegionServer: public DataNodeComponent{
private:
    std::unordered_map<u_int64_t, std::string> regionEngineMap;
    std::vector<PipeRing*> enginePipes;
    u_int64_t requestID;
    std::istream* in;

    bool getRequest(std::string& request);
    std::vector<MitoSignal*> parseRequests(std::string& rawRequest);
    void sendRegionRequest(MitoSignal* signal);
    void handleFailure(std::string request);
    void handleNewRegion(u_int64_t regionID);
public:
    RegionServer(const std::string& name, const std::string& logPath, const u_int64_t id);
    ~RegionServer() = default;
    void init(DataNodeContext& cfg) override;
    void run() override;
};
