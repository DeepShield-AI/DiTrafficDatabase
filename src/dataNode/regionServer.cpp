#include "regionServer.hpp"



RegionServer::RegionServer(const std::string& name, const std::string& logPath, const u_int64_t id):DataNodeComponent("RegionServer", name, logPath, id){
    this->regionEngineMap = std::unordered_map<u_int64_t, std::string>();
    this->enginePipes = std::vector<PipeRing*>();
    this->requestID = 0;
}

std::string RegionServer::getRequest(){
    std::string request;
    std::cout << "Enter request: ";
    std::cin >> request;
    return request;
}

std::vector<MitoSignal*> RegionServer::parseRequests(std::string& rawRequest){
    json j = json::parse(rawRequest);
    auto requests = j.get<DictList>();
    std::vector<MitoSignal*> signals;
    for(auto request:requests){
        if(request.find("type") == request.end() || request.find("regionID") == request.end()){
            json jr = request;
            this->log("Invalid request format: " + jr.dump());
            continue;
        }
        if(this->regionEngineMap.find(std::stoull(request["regionID"])) == this->regionEngineMap.end()){
            if(request["type"] != "ADMIN" || request.find("operation") == request.end() || request["operation"] != "CREATE"){
                json jr = request;
                this->log("Region " + request["regionID"] + " not found. Only ADMIN CREATE requests are allowed for new regions. Request: " + jr.dump());
                this->handleFailure(jr.dump());
                continue;
            }
        }
        if(request["type"] == "WRITE"){
            if(request.find("timestamp") == request.end() || request.find("jsonData") == request.end()){
                json jr = request;
                this->log("Invalid WRITE request format: " + jr.dump());
                continue;
            }
            WriteRequest writeReq = {
                .timestamp = std::stoull(request["timestamp"]),
                .jsonSize = request["jsonData"].size(),
            };
            // std::string* jsonData = new std::string(request["jsonData"]);
            // writeReq.jsonData = (const u_int8_t*)jsonData->c_str();
            writeReq.jsonData = (const u_int8_t*)new char[writeReq.jsonSize];
            memcpy((void*)writeReq.jsonData, request["jsonData"].c_str(), writeReq.jsonSize);

            MitoSignal* signal = new MitoSignal{
                .regionID = std::stoull(request["regionID"]),
                .request = Request{
                    .header = RequestHeader{
                        .request_id = this->requestID++,
                        .timestamp_ns = (u_int64_t)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count(),
                    },
                    .type = RequestType::WRITE,
                    .payload = writeReq,
                },
            };
            signals.push_back(signal);
        }else if (request["type"] == "ADMIN"){
            if(request.find("operation") == request.end()){
                json jr = request;
                this->log("Invalid ADMIN request format: " + jr.dump());
                continue;
            }
            RegionAdminRequest adminReq;
            if(request["operation"] == "CREATE"){
                if(request.find("regionName") == request.end() || request.find("expr") == request.end() || request.find("attrs") == request.end()){
                    json jr = request;
                    this->log("Invalid ADMIN CREATE request format: " + jr.dump());
                    continue;
                }
                adminReq.op = RegionOperation::CREATE;
                adminReq.create.name = (const u_int8_t*)new char[request["regionName"].size()];
                memcpy((void*)adminReq.create.name, request["regionName"].c_str(), request["regionName"].size());
                adminReq.create.name_size = request["regionName"].size();
                adminReq.create.partition_expr = (const u_int8_t*)new char[request["expr"].size()];
                memcpy((void*)adminReq.create.partition_expr, request["expr"].c_str(), request["expr"].size());
                adminReq.create.partition_expr_size = request["expr"].size();
                adminReq.create.attrs_json = (const u_int8_t*)new char[request["attrs"].size()];
                memcpy((void*)adminReq.create.attrs_json, request["attrs"].c_str(), request["attrs"].size());
                adminReq.create.attrs_json_size = request["attrs"].size();
                this->handleNewRegion(std::stoull(request["regionID"]));
            }else if (request["operation"] == "OPEN"){
                adminReq.op = RegionOperation::OPEN;
                adminReq.open.force = false;
            }else if (request["operation"] == "CLOSE"){
                adminReq.op = RegionOperation::CLOSE;
                adminReq.close.force = false;
            }else if (request["operation"] == "DROP"){
                adminReq.op = RegionOperation::DROP;
                adminReq.drop.keep_files = false;
            }else{
                json jr = request;
                this->log("Invalid ADMIN operation: " + jr.dump());
                continue;  
            }
            MitoSignal* signal = new MitoSignal{
                .regionID = std::stoull(request["regionID"]),
                .request = Request{
                    .header = RequestHeader{
                        .request_id = this->requestID++,
                        .timestamp_ns = (u_int64_t)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count(),
                    },
                    .type = RequestType::REGION_ADMIN,
                    .payload = adminReq,
                },
            };
            signals.push_back(signal);
        }else{
            json jr = request;
            this->log("Unknown request type: " + jr.dump());
            continue;
        }
    }
    return signals;
}

void RegionServer::sendRegionRequest(MitoSignal* signal){
    u_int64_t regionID = signal->regionID;
    if(this->regionEngineMap.find(regionID) == this->regionEngineMap.end()){
        this->log("Region " + std::to_string(regionID) + " not found when sending request.");
        delete signal;
        return;
    }
    this->enginePipes[0]->put((void*)signal);
    this->log("Sent request " + std::to_string(signal->request.header.request_id) + " for region " + std::to_string(regionID) + " to engine " + this->regionEngineMap[regionID]);
}

void RegionServer::handleFailure(std::string request){
    this->log("Handling failed request: " + request);
}

void RegionServer::handleNewRegion(u_int64_t regionID){
    this->regionEngineMap[regionID] = "MitoEngine"; // default engine
}

void RegionServer::init(DataNodeContext& cfg){
    this->enginePipes.push_back(cfg.serverEnginePipe);
}

void RegionServer::run(){
    this->start();
    while(this->isRunning()){
        std::string rawRequest = this->getRequest();
        auto signals = this->parseRequests(rawRequest);
        for(auto& signal : signals){
            this->sendRegionRequest(signal);
        }
    }
}